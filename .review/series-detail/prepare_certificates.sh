#!/bin/bash
# Run only in the reviewed remote job. Keys stay outside uploaded proof.
set -euo pipefail
task_cert_dir=$1
mkdir -p "$task_cert_dir"
chmod 700 "$task_cert_dir"
umask 077
openssl req -x509 -newkey rsa:2048 -sha256 -nodes -days 2 \
  -subj "/CN=Nuvio isolated metadata fixture ${GITHUB_RUN_ID:-local}" \
  -addext 'basicConstraints=critical,CA:TRUE' \
  -addext 'keyUsage=critical,keyCertSign,cRLSign' \
  -keyout "$task_cert_dir/ca.key" -out "$task_cert_dir/ca.pem"
openssl req -new -newkey rsa:2048 -sha256 -nodes \
  -subj '/CN=catalog.nuvio.tv' -keyout "$task_cert_dir/server.key" \
  -out "$task_cert_dir/server.csr"
cat > "$task_cert_dir/server.ext" <<'EOF'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:catalog.nuvio.tv,DNS:v3-cinemeta.strem.io
EOF
openssl x509 -req -sha256 -days 2 -in "$task_cert_dir/server.csr" \
  -CA "$task_cert_dir/ca.pem" -CAkey "$task_cert_dir/ca.key" -CAcreateserial \
  -extfile "$task_cert_dir/server.ext" -out "$task_cert_dir/server.pem"
openssl verify -CAfile "$task_cert_dir/ca.pem" -verify_hostname catalog.nuvio.tv "$task_cert_dir/server.pem"
openssl verify -CAfile "$task_cert_dir/ca.pem" -verify_hostname v3-cinemeta.strem.io "$task_cert_dir/server.pem"
