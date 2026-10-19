#!/usr/bin/env python3
"""Verify this test APK's actual v2 signature/digest, manifest, CRC and ARM ELFs.

Requires Python cryptography. Deliberately supports the persistent test key's
single-signer RSA PKCS#1 v1.5/SHA256 APK-v2 profile, rejecting other profiles.
This does not replace Android CI's apksigner checks of v1/v3/platform policy.
Expected version overrides are explicit; they are never inferred from a file.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa

SIGNER = '862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def number(data, offset, code):
    size = struct.calcsize(code)
    require(0 <= offset <= len(data) - size, 'Truncated binary field')
    return struct.unpack_from('<' + code, data, offset)[0]


def u16(data, offset):
    return number(data, offset, 'H')


def u32(data, offset):
    return number(data, offset, 'I')


def u64(data, offset):
    return number(data, offset, 'Q')


def lp(data, offset=0):
    length = u32(data, offset)
    end = offset + 4 + length
    require(end <= len(data), 'Truncated length-prefixed field')
    return data[offset + 4:end], end


def sequence(data):
    records = []
    offset = 0
    while offset < len(data):
        value, offset = lp(data, offset)
        records.append(value)
    require(offset == len(data), 'Invalid sequence boundary')
    return records


def records_by_algorithm(data):
    values = {}
    for record in sequence(data):
        algorithm = u32(record, 0)
        value, end = lp(record, 4)
        require(end == len(record) and algorithm not in values, 'Invalid/duplicate algorithm record')
        values[algorithm] = value
    return values


def v2_verify(data, expected_signer):
    # EOCD can legally contain its magic in a comment, so accept only a record
    # whose complete declared comment ends at the physical end of the file.
    end = len(data)
    eocd = None
    for position in range(end - 22, max(-1, end - 65558), -1):
        if data[position:position + 4] == b'PK\x05\x06' and position + 22 + u16(data, position + 20) == end:
            eocd = position
            break
    require(eocd is not None, 'ZIP EOCD not found')
    require(u16(data, eocd + 4) == 0 and u16(data, eocd + 6) == 0, 'Multi-disk ZIP unsupported')
    central = u32(data, eocd + 16)
    require(central + u32(data, eocd + 12) == eocd, 'Invalid central-directory boundary/ZIP64')
    require(data[central - 16:central] == b'APK Sig Block 42', 'APK signing block not found')
    size = u64(data, central - 24)
    start = central - size - 8
    require(start >= 0 and size >= 24 and u64(data, start) == size, 'Invalid APK signing-block sizes')
    position = start + 8
    pairs = {}
    while position < central - 24:
        length = u64(data, position)
        position += 8
        require(length >= 4 and position + length <= central - 24, 'Invalid signing-block pair')
        identifier = u32(data, position)
        require(identifier not in pairs, 'Duplicate signing-block ID')
        pairs[identifier] = data[position + 4:position + length]
        position += length
    require(position == central - 24 and 0x7109871a in pairs, 'APK v2 block missing/invalid')
    signers, consumed = lp(pairs[0x7109871a])
    require(consumed == len(pairs[0x7109871a]), 'Trailing APK v2 bytes')
    signers = sequence(signers)
    require(len(signers) == 1, 'Expected exactly one APK v2 signer')
    signer = signers[0]
    signed, offset = lp(signer)
    signatures, offset = lp(signer, offset)
    public_der, offset = lp(signer, offset)
    require(offset == len(signer), 'Trailing signer bytes')
    digests, offset = lp(signed)
    certificates, offset = lp(signed, offset)
    attributes, offset = lp(signed, offset)
    # apksig's test signing profile appends an empty extension/padding word.
    # It is still authenticated: verify() below uses the entire signed blob.
    require(offset == len(signed) or signed[offset:] == b'\x00' * 4,
            'Unsupported authenticated signed-data extension')
    # Validate the attribute framing as well; the complete bytes remain covered
    # by the RSA signature even though their policy is left to apksigner.
    sequence(attributes)
    digests = records_by_algorithm(digests)
    signatures = records_by_algorithm(signatures)
    require(list(digests) == list(signatures) == [0x0103],
            'Expected RSA PKCS#1 v1.5/SHA256 single-algorithm APK v2 profile')
    certificates = sequence(certificates)
    require(len(certificates) == 1, 'Expected persistent self-signed test certificate only')
    certificate_der = certificates[0]
    fingerprint = hashlib.sha256(certificate_der).hexdigest()
    require(fingerprint == expected_signer, 'Persistent signing certificate differs')
    certificate = x509.load_der_x509_certificate(certificate_der)
    key = serialization.load_der_public_key(public_der)
    require(isinstance(key, rsa.RSAPublicKey) and key.key_size >= 2048, 'Expected RSA2048+ test key')
    require(certificate.public_key().public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo) == public_der,
            'Signer public key does not match its certificate')
    # This verifies ownership of the private key, not merely certificate bytes.
    key.verify(signatures[0x0103], signed, padding.PKCS1v15(), hashes.SHA256())
    fixed_eocd = bytearray(data[eocd:])
    struct.pack_into('<I', fixed_eocd, 16, start)
    chunk_digests = []
    for section in (memoryview(data)[:start], memoryview(data)[central:eocd], memoryview(fixed_eocd)):
        for offset in range(0, len(section), 1024 * 1024):
            chunk = section[offset:offset + 1024 * 1024]
            digest = hashlib.sha256(b'\xa5' + struct.pack('<I', len(chunk)))
            digest.update(chunk)
            chunk_digests.append(digest.digest())
    content = hashlib.sha256(b'\x5a' + struct.pack('<I', len(chunk_digests)) + b''.join(chunk_digests)).digest()
    require(content == digests[0x0103], 'APK v2 content digest mismatch: APK bytes were changed')
    return fingerprint


def manifest_identity(data):
    require(u16(data, 0) == 3 and u32(data, 4) == len(data), 'Invalid binary Android manifest')
    position = u16(data, 2)
    strings = []
    identity = None
    while position < len(data):
        kind = u16(data, position)
        header = u16(data, position + 2)
        size = u32(data, position + 4)
        require(size >= header >= 8 and position + size <= len(data), 'Invalid binary XML chunk')
        if kind == 1:
            count = u32(data, position + 8)
            utf8 = u32(data, position + 16) & 0x100
            base = position + u32(data, position + 20)
            require(position + header + 4 * count <= position + size, 'Invalid string-pool index')
            strings = []
            def length(offset, wide=False):
                value = u16(data, offset) if wide else data[offset]
                offset += 2 if wide else 1
                mask = 0x8000 if wide else 0x80
                if value & mask:
                    second = u16(data, offset) if wide else data[offset]
                    offset += 2 if wide else 1
                    value = ((value & (mask - 1)) << (16 if wide else 8)) | second
                return value, offset
            for index in range(count):
                offset = base + u32(data, position + header + 4 * index)
                if utf8:
                    _, offset = length(offset)
                    width, offset = length(offset)
                    require(offset + width + 1 <= position + size, 'Truncated UTF8 XML string')
                    strings.append(data[offset:offset + width].decode('utf-8'))
                else:
                    width, offset = length(offset, True)
                    require(offset + 2 * width + 2 <= position + size, 'Truncated UTF16 XML string')
                    strings.append(data[offset:offset + 2 * width].decode('utf-16-le'))
        elif kind == 0x102 and strings[u32(data, position + 20)] == 'manifest':
            require(identity is None, 'Duplicate manifest root')
            start = position + 16 + u16(data, position + 24)
            step = u16(data, position + 26)
            count = u16(data, position + 28)
            require(step >= 20 and start + step * count <= position + size, 'Invalid manifest attributes')
            identity = {}
            for index in range(count):
                offset = start + index * step
                name = strings[u32(data, offset + 4)]
                namespace = u32(data, offset)
                if name == 'package':
                    require(namespace == 0xffffffff, 'Unexpected package namespace')
                elif name in ('versionCode', 'versionCodeMajor', 'versionName'):
                    require(strings[namespace] == 'http://schemas.android.com/apk/res/android', 'Unexpected version namespace')
                else:
                    continue
                require(name not in identity, 'Duplicate identity attribute')
                raw = u32(data, offset + 8)
                value = u32(data, offset + 16)
                value_type = data[offset + 15]
                identity[name] = strings[raw] if raw != 0xffffffff else strings[value] if value_type == 3 else value
        position += size
    require(identity is not None, 'Manifest root missing')
    require(not identity.get('versionCodeMajor', 0), 'Unexpected major version code')
    return identity


def elf_loads(data, arm64):
    require(data[:4] == b'\x7fELF' and data[5] == 1, 'Expected little-endian ELF library')
    require(data[4] == (2 if arm64 else 1), 'Wrong ELF class for ABI')
    require(u16(data, 18) == (183 if arm64 else 40), 'Wrong ELF CPU for ABI')
    if not arm64:
        return []
    start, step, count = u64(data, 32), u16(data, 54), u16(data, 56)
    require(step >= 56 and start + step * count <= len(data), 'Invalid ELF program headers')
    loads = []
    for index in range(count):
        offset = start + index * step
        if u32(data, offset) != 1:
            continue
        file_offset, address, alignment = u64(data, offset + 8), u64(data, offset + 16), u64(data, offset + 48)
        require(alignment >= 16384 and alignment & (alignment - 1) == 0, 'ARM64 LOAD alignment below 16KB')
        require(file_offset % 16384 == address % 16384, 'ARM64 LOAD offset/address mismatch for 16KB pages')
        loads.append({'offset': file_offset, 'vaddr': address, 'alignment': alignment})
    require(loads, 'No ARM64 LOAD segments')
    return loads


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    parser.add_argument('--expected-sha256', required=True)
    parser.add_argument('--expected-bytes', type=int)
    parser.add_argument('--package', default='space.nuvio.nativelegacy.touch')
    parser.add_argument('--version-code', type=int, default=20074)
    parser.add_argument('--version-name', default='2.0.74-test-logs64')
    parser.add_argument('--certificate-sha256', default=SIGNER)
    parser.add_argument('--expected-arm64-libraries', type=int, default=6)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    data = args.apk.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    require(digest == args.expected_sha256.lower(), 'Downloaded APK SHA256 differs from expected digest')
    if args.expected_bytes is not None:
        require(len(data) == args.expected_bytes, 'Downloaded APK length differs')
    fingerprint = v2_verify(data, args.certificate_sha256.lower())
    with zipfile.ZipFile(args.apk) as archive:
        names = archive.namelist()
        require(len(names) == len(set(names)), 'Duplicate ZIP entry names')
        require(archive.testzip() is None, 'ZIP CRC check failed')
        identity = manifest_identity(archive.read('AndroidManifest.xml'))
        require(identity.get('package') == args.package, 'Unexpected manifest package')
        require(int(identity.get('versionCode', -1)) == args.version_code, 'Unexpected manifest version code')
        require(identity.get('versionName') == args.version_name, 'Unexpected manifest version name')
        libraries = [name for name in names if name.startswith('lib/') and name.endswith('.so')]
        abis = {name.split('/')[1] for name in libraries}
        require(abis == {'arm64-v8a', 'armeabi-v7a'}, 'Expected precisely ARM64 and ARMv7 ABIs')
        for abi in abis:
            require('lib/' + abi + '/libmain.so' in names, 'Main native library missing for ' + abi)
        load_segments = {}
        for name in libraries:
            arm64 = name.startswith('lib/arm64-v8a/')
            segments = elf_loads(archive.read(name), arm64)
            if arm64:
                load_segments[name] = segments
        require(len(load_segments) == args.expected_arm64_libraries, 'Unexpected ARM64 library count')
    result = {
        'apk_sha256': digest, 'apk_bytes': len(data),
        'package': identity['package'], 'version_code': int(identity['versionCode']), 'version_name': identity['versionName'],
        'certificate_sha256': fingerprint, 'apk_v2_signature_verified': True,
        'apk_v2_content_digest_verified': True, 'manifest_verified': True, 'zip_crc_verified': True,
        'abis': sorted(abis), 'arm64_16kb_load_segments': load_segments,
        'verification_scope': 'APK v2 RSA/SHA256 cryptographic signature and content digest, identity, CRC and ARM ELF alignment. Android CI separately checks apksigner platform/v1/v3 policy.',
    }
    body = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.write_text(body)
    print(body, end='')


if __name__ == '__main__':
    main()
