# Windows PowerShell 5.1. Account data stays on this computer.
[CmdletBinding(DefaultParameterSetName = 'Backup')]
param(
    [Parameter(ParameterSetName = 'Backup')][switch]$BackupOnly,
    [Parameter(Mandatory = $true, ParameterSetName = 'Transfer')][switch]$Transfer,
    [Parameter(Mandatory = $true, ParameterSetName = 'Restore')][switch]$Restore,
    [Parameter(Mandatory = $true, ParameterSetName = 'Restore')][string]$BackupFolder,
    [Parameter(ParameterSetName = 'Restore')][switch]$OriginalApk,
    [string]$Device,
    [string]$AdbPath
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$script:Package = 'space.nuvio.nativelegacy.touch'
$script:ApkUrl = 'https://github.com/ghbarker/nuvio-native-legacy/releases/download/tv-mobile-test-languages-481/Nuvio-TV-Mobile-Test.apk'
$script:ApkSha256 = '179fc2763cbddeec302c9dbfbb5f7a29e1165e8600ce4118bfb8bba0a7d38ed6'
$script:AdbDeviceArgs = @()
$script:RetainedBackup = ''
$script:RestoreMayBeNeeded = $false

function Quote-ProcessArgument([string]$Value) {
    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
    $escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}

function New-Process([string]$Program, [string[]]$Arguments, [bool]$InputPipe = $false) {
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $Program
    $info.Arguments = (($Arguments | ForEach-Object { Quote-ProcessArgument $_ }) -join ' ')
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.RedirectStandardInput = $InputPipe
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    if (-not $process.Start()) { throw 'Could not start the local tool.' }
    return $process
}

function Invoke-TextTool([string]$Program, [string[]]$Arguments) {
    $process = New-Process $Program $Arguments
    try {
        $outTask = $process.StandardOutput.ReadToEndAsync()
        $errTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        return [pscustomobject]@{
            Code = $process.ExitCode
            StdOut = $outTask.GetAwaiter().GetResult()
            StdErr = $errTask.GetAwaiter().GetResult()
        }
    } finally { $process.Dispose() }
}

function Invoke-Adb([string[]]$Arguments) {
    return Invoke-TextTool $script:Adb ($script:AdbDeviceArgs + $Arguments)
}

function Require-AdbSuccess([string[]]$Arguments, [string]$Failure) {
    $result = Invoke-Adb $Arguments
    if ($result.Code -ne 0) { throw $Failure }
    return $result
}

function Stop-TestApp {
    $null = Require-AdbSuccess @('shell', 'am', 'force-stop', $script:Package) 'Could not stop the test app. No data was removed.'
}

function Require-DebugApp {
    $null = Require-AdbSuccess @('shell', 'run-as', $script:Package, 'true') 'The installed test app does not permit local ADB backup. No data was removed.'
}

function Capture-AdbArchive([string]$Destination) {
    # Do not replace this with >, Out-File, or a PowerShell pipeline: PS5.1 corrupts binary output.
    # Raw exec-out does not reliably report the remote exit code. A random footer
    # is written only after tar succeeds, then removed from the verified archive.
    $token = 'NUVIO_BACKUP_OK_' + [guid]::NewGuid().ToString('N')
    $footer = [System.Text.Encoding]::ASCII.GetBytes($token)
    $command = "run-as $($script:Package) sh -c 'tar -cf - files/dados && printf $token'"
    $process = New-Process $script:Adb ($script:AdbDeviceArgs + @('exec-out', $command))
    $file = $null
    try {
        $errTask = $process.StandardError.ReadToEndAsync()
        $file = [System.IO.File]::Open($Destination, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
        $process.StandardOutput.BaseStream.CopyTo($file)
        $file.Flush()
        if ($file.Length -le $footer.Length) { throw 'Remote backup did not finish successfully. No data was removed.' }
        $file.Position = $file.Length - $footer.Length
        $actualFooter = New-Object byte[] $footer.Length
        if ($file.Read($actualFooter, 0, $actualFooter.Length) -ne $footer.Length -or
            [System.Text.Encoding]::ASCII.GetString($actualFooter) -cne $token) {
            throw 'Remote backup did not finish successfully. No data was removed.'
        }
        $file.SetLength($file.Length - $footer.Length)
        $file.Dispose()
        $file = $null
        $process.WaitForExit()
        $captureError = $errTask.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 0 -or $captureError.Trim().Length -gt 0) { throw 'Local app backup failed. No data was removed.' }
    } catch {
        if (-not $process.HasExited) { $process.Kill() }
        throw
    } finally {
        if ($null -ne $file) { $file.Dispose() }
        $process.Dispose()
    }
}

function Restore-AdbArchive([string]$Source) {
    Write-Host 'Restoring the verified local account backup. Keep the device connected.'
    # Feed the archive directly to run-as; never place credentials in /sdcard or /data/local/tmp.
    $process = New-Process $script:Adb ($script:AdbDeviceArgs + @('exec-in', 'run-as', $script:Package, 'tar', '-xf', '-')) $true
    $file = $null
    try {
        $outTask = $process.StandardOutput.ReadToEndAsync()
        $errTask = $process.StandardError.ReadToEndAsync()
        $file = [System.IO.File]::OpenRead($Source)
        $file.CopyTo($process.StandardInput.BaseStream)
        $process.StandardInput.BaseStream.Flush()
        $process.StandardInput.Close()
        $file.Dispose()
        $file = $null
        $process.WaitForExit()
        $null = $outTask.GetAwaiter().GetResult()
        $null = $errTask.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 0) { throw 'Restoring the local backup failed. The original backup is retained.' }
    } catch {
        if (-not $process.HasExited) { $process.Kill() }
        throw
    } finally {
        if ($null -ne $file) { $file.Dispose() }
        $process.Dispose()
    }
}

function File-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Reject-ReparsePoint([string]$Path) {
    $item = Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'A backup path is a link. Use the original local backup folder.'
    }
}

function Validate-Archive([string]$Archive) {
    Reject-ReparsePoint $Archive
    $names = Invoke-TextTool $script:TarPath @('-tf', $Archive)
    $types = Invoke-TextTool $script:TarPath @('-tvf', $Archive)
    if ($names.Code -ne 0 -or $types.Code -ne 0) { throw 'The backup archive is invalid. No data was removed.' }
    $entries = @($names.StdOut -split '\r?\n' | Where-Object { $_.Length -gt 0 })
    $details = @($types.StdOut -split '\r?\n' | Where-Object { $_.Length -gt 0 })
    if ($entries.Count -eq 0 -or $entries.Count -ne $details.Count) { throw 'The backup archive could not be fully checked.' }
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $entries) {
        $name = $entry.TrimEnd('/')
        if ($name -cnotmatch '^files/dados(?:/|$)' -or
            $name -match '(^|/)\.\.?(/|$)|[\\:\x00-\x1f*?"<>|]|[. ](/|$)' -or
            $name -match '(?i)(^|/)(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|/|$)' -or
            -not $seen.Add($name)) {
            throw 'The backup contains an unsafe or duplicate path. No data was removed.'
        }
    }
    foreach ($detail in $details) {
        if ($detail -notmatch '^[d-]') { throw 'The backup contains a link or special file. No data was removed.' }
    }
    if (-not $seen.Contains('files/dados')) { throw 'The backup is missing the app data directory.' }
}

function Get-ArchiveSnapshot([string]$Archive, [string]$ExtractDirectory) {
    Validate-Archive $Archive
    if (Test-Path -LiteralPath $ExtractDirectory) { throw 'Backup verification folder already exists.' }
    $null = New-Item -ItemType Directory -Path $ExtractDirectory
    $result = Invoke-TextTool $script:TarPath @('-xf', $Archive, '-C', $ExtractDirectory)
    if ($result.Code -ne 0) { throw 'Could not extract the local backup for verification. No data was removed.' }
    $base = (Get-Item -LiteralPath $ExtractDirectory).FullName
    $data = Join-Path $base 'files/dados'
    if (-not (Test-Path -LiteralPath $data -PathType Container)) { throw 'The backup did not contain an app data directory.' }
    $inventory = New-Object System.Collections.Generic.List[object]
    $inventory.Add([pscustomobject]@{ Path = 'files/dados'; Kind = 'directory'; Sha256 = '' })
    foreach ($item in @(Get-ChildItem -LiteralPath $data -Force -Recurse)) {
        Reject-ReparsePoint $item.FullName
        $relative = $item.FullName.Substring($base.Length + 1).Replace('\', '/')
        $kind = 'file'
        $hash = ''
        if ($item.PSIsContainer) { $kind = 'directory' } else { $hash = File-Sha256 $item.FullName }
        $inventory.Add([pscustomobject]@{ Path = $relative; Kind = $kind; Sha256 = $hash })
    }
    return @($inventory | Sort-Object -Property Path)
}

function Compare-Inventory([object[]]$Expected, [object[]]$Actual) {
    if ($Expected.Count -ne $Actual.Count) { throw 'Data verification did not match the backup. Do not launch the app; the original backup is retained.' }
    $expectedSorted = @($Expected | Sort-Object -Property Path)
    $actualSorted = @($Actual | Sort-Object -Property Path)
    for ($i = 0; $i -lt $expectedSorted.Count; $i++) {
        $a = $expectedSorted[$i]
        $b = $actualSorted[$i]
        if ($a.Path -cne $b.Path -or $a.Kind -cne $b.Kind -or $a.Sha256 -cne $b.Sha256) {
            throw 'Data verification did not match the backup. Do not launch the app; the original backup is retained.'
        }
    }
}

function Save-OriginalApk([string]$Folder) {
    $result = Require-AdbSuccess @('shell', 'pm', 'path', $script:Package) 'Could not locate the installed APK. No data was removed.'
    $paths = @($result.StdOut -split '\r?\n' | Where-Object { $_.Length -gt 0 })
    if ($paths.Count -ne 1 -or $paths[0] -notmatch '^package:(/data/app/[A-Za-z0-9._/+=~-]+\.apk)$') {
        throw 'Expected one complete installed APK. Split APK installations are not handled; no data was removed.'
    }
    $remote = $Matches[1]
    $saved = Join-Path $Folder 'original-installed.apk'
    $null = Require-AdbSuccess @('pull', $remote, $saved) 'Could not save the original installed APK. No data was removed.'
    $sourceHash = Require-AdbSuccess @('shell', 'sha256sum', $remote) 'Could not verify the original APK checksum. No data was removed.'
    if ($sourceHash.StdOut -notmatch '^([0-9a-fA-F]{64})\s' -or (File-Sha256 $saved) -ine $Matches[1]) {
        throw 'The saved original APK checksum did not match the installed file. No data was removed.'
    }
    return File-Sha256 $saved
}

function New-VerifiedBackup([switch]$AllowEmpty) {
    Write-Host 'Backing up app data and the original installed APK. Keep the device connected.'
    Stop-TestApp
    Require-DebugApp
    $folder = Join-Path $script:PrivateRoot ('backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
    $null = New-Item -ItemType Directory -Path $folder
    $archive = Join-Path $folder 'accounts-data.tar'
    Capture-AdbArchive $archive
    $entries = @(Get-ArchiveSnapshot $archive (Join-Path $folder 'verified-copy'))
    if (-not $AllowEmpty -and @($entries | Where-Object { $_.Kind -eq 'file' }).Count -eq 0) { throw 'The backup contains no saved app files. No data was removed.' }
    $originalHash = Save-OriginalApk $folder
    $manifest = [pscustomobject]@{ Version = 1; Package = $script:Package; ArchiveSha256 = (File-Sha256 $archive); OriginalApkSha256 = $originalHash; Entries = $entries }
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $folder 'manifest.json') -Encoding UTF8
    $null = Read-VerifiedBackup $folder
    Write-Host ('Verified local backup: ' + $folder)
    Write-Host 'Keep this folder. It contains private account data; do not upload it.'
    return $folder
}

function Read-VerifiedBackup([string]$Folder) {
    Reject-ReparsePoint $Folder
    $archive = Join-Path $Folder 'accounts-data.tar'
    $manifestFile = Join-Path $Folder 'manifest.json'
    $original = Join-Path $Folder 'original-installed.apk'
    Reject-ReparsePoint $manifestFile
    Reject-ReparsePoint $original
    $manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
    if ($manifest.Version -ne 1 -or $manifest.Package -cne $script:Package -or
        $manifest.ArchiveSha256 -notmatch '^[0-9a-f]{64}$' -or
        (File-Sha256 $archive) -cne $manifest.ArchiveSha256 -or
        $manifest.OriginalApkSha256 -notmatch '^[0-9a-f]{64}$' -or
        (File-Sha256 $original) -cne $manifest.OriginalApkSha256) { throw 'Backup identity or checksum verification failed. No data was removed.' }
    $check = Join-Path $script:PrivateRoot ('check-' + [guid]::NewGuid().ToString('N'))
    $actual = @(Get-ArchiveSnapshot $archive $check)
    Compare-Inventory @($manifest.Entries) $actual
    return [pscustomobject]@{ Folder = $Folder; Archive = $archive; OriginalApk = $original; OriginalApkSha256 = $manifest.OriginalApkSha256; Entries = @($manifest.Entries) }
}

function Initialize-PrivateFolder {
    $script:PrivateRoot = Join-Path $env:LOCALAPPDATA 'Nuvio-Test-Account-Transfer'
    if (-not (Test-Path -LiteralPath $script:PrivateRoot)) { $null = New-Item -ItemType Directory -Path $script:PrivateRoot }
    Reject-ReparsePoint $script:PrivateRoot
    $acl = New-Object System.Security.AccessControl.DirectorySecurity
    $acl.SetAccessRuleProtection($true, $false)
    $userSid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User
    $acl.SetOwner($userSid)
    foreach ($sid in @($userSid.Value, 'S-1-5-18', 'S-1-5-32-544')) {
        $identity = [System.Security.Principal.SecurityIdentifier]::new($sid)
        $rule = [System.Security.AccessControl.FileSystemAccessRule]::new($identity, 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')
        $acl.AddAccessRule($rule)
    }
    Set-Acl -LiteralPath $script:PrivateRoot -AclObject $acl
}

function Initialize-Adb {
    if ($AdbPath) {
        $script:Adb = (Get-Item -LiteralPath $AdbPath).FullName
    } else {
        $existing = Get-Command adb.exe -ErrorAction SilentlyContinue
        if ($null -ne $existing) { $script:Adb = $existing.Source } else {
            $script:Adb = Join-Path $script:PrivateRoot 'platform-tools/adb.exe'
            if (-not (Test-Path -LiteralPath $script:Adb -PathType Leaf)) {
                Write-Host 'Downloading official Google Android platform-tools to the local helper folder.'
                $zip = Join-Path $script:PrivateRoot 'platform-tools-windows.zip'
                Invoke-WebRequest -UseBasicParsing -Uri 'https://dl.google.com/android/repository/platform-tools-latest-windows.zip' -OutFile $zip
                Expand-Archive -LiteralPath $zip -DestinationPath $script:PrivateRoot -Force
            }
        }
    }
    $result = Invoke-TextTool $script:Adb @('devices')
    if ($result.Code -ne 0) { throw 'ADB could not start. No data was removed.' }
    $authorized = @()
    foreach ($line in ($result.StdOut -split '\r?\n')) {
        if ($line -match '^(\S+)\s+device(?:\s|$)') { $authorized += $Matches[1] }
    }
    if ($Device) {
        if ($Device -notmatch '^[A-Za-z0-9._:\[\]-]+$' -or $authorized -notcontains $Device) { throw 'The selected device is not connected and authorized. Accept USB debugging on the device.' }
        $selected = $Device
    } elseif ($authorized.Count -eq 1) { $selected = $authorized[0] } else {
        throw 'Connect exactly one device and accept its USB debugging prompt, or specify -Device SERIAL. No data was removed.'
    }
    $script:AdbDeviceArgs = @('-s', $selected)
}

function Get-FixedApk {
    if ($script:ApkUrl -notmatch '^https://' -or $script:ApkSha256 -notmatch '^[0-9a-fA-F]{64}$') { throw 'The fixed APK download has not been configured. Your verified backup is retained.' }
    $apk = Join-Path $script:PrivateRoot 'Nuvio-TV-Mobile-Test-stable.apk'
    if (-not (Test-Path -LiteralPath $apk) -or (File-Sha256 $apk) -ine $script:ApkSha256) {
        Write-Host 'Downloading the fixed, consistently signed test APK.'
        $partial = Join-Path $script:PrivateRoot ('apk-' + [guid]::NewGuid().ToString('N') + '.partial')
        Invoke-WebRequest -UseBasicParsing -Uri $script:ApkUrl -OutFile $partial
        if ((File-Sha256 $partial) -ine $script:ApkSha256) { throw 'APK checksum verification failed. No app data was removed.' }
        Move-Item -LiteralPath $partial -Destination $apk -Force
    }
    Reject-ReparsePoint $apk
    return $apk
}

function Verify-InstalledData([object]$Backup) {
    Write-Host 'Checking every restored file and directory before opening the app.'
    Stop-TestApp
    Require-DebugApp
    $folder = Join-Path $script:PrivateRoot ('restored-check-' + [guid]::NewGuid().ToString('N'))
    $null = New-Item -ItemType Directory -Path $folder
    $archive = Join-Path $folder 'restored-data.tar'
    Capture-AdbArchive $archive
    $actual = @(Get-ArchiveSnapshot $archive (Join-Path $folder 'verified-copy'))
    Compare-Inventory $Backup.Entries $actual
    Write-Host 'Every backed-up file and directory matches on the device. Accounts and settings have been restored.'
}

function Install-Succeeded([object]$Result) {
    return $Result.Code -eq 0 -and $Result.StdOut -match '(?m)^Success\s*$'
}

function Launch-TestApp {
    $null = Require-AdbSuccess @('shell', 'am', 'start', '-n', ($script:Package + '/space.nuvio.nativelegacy.NuvioActivity')) 'Data was verified, but the app could not launch automatically. Open Nuvio TV/Mobile Test yourself.'
}

function Start-NuvioTransfer {
    if ($env:OS -ne 'Windows_NT') { throw 'Run this helper on your Windows computer.' }
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $tar = Get-Command tar.exe -ErrorAction SilentlyContinue
    if ($null -eq $tar) { throw 'This helper needs Windows built-in tar.exe (Windows 10/11). No device data was changed.' }
    $script:TarPath = $tar.Source
    Initialize-PrivateFolder
    Initialize-Adb
    if ($Restore) {
        $backup = Read-VerifiedBackup (Get-Item -LiteralPath $BackupFolder).FullName
        $script:RetainedBackup = $backup.Folder
        if ($OriginalApk) { $apk = $backup.OriginalApk; $expectedApkHash = $backup.OriginalApkSha256 } else { $apk = Get-FixedApk; $expectedApkHash = $script:ApkSha256 }
        $installed = Require-AdbSuccess @('shell', 'pm', 'list', 'packages', $script:Package) 'Could not check the installed app. No data was removed.'
        $installedNames = @($installed.StdOut -split '\r?\n')
        if ($installedNames -ccontains ('package:' + $script:Package)) {
            Require-DebugApp
            $dataCommand = "run-as $($script:Package) sh -c 'if [ -d files/dados ]; then printf EXISTS; else printf ABSENT; fi'"
            $existingData = Require-AdbSuccess @('shell', $dataCommand) 'Could not check existing app data. No data was removed.'
            if ($existingData.StdOut -ceq 'EXISTS') { $null = New-VerifiedBackup -AllowEmpty }
            elseif ($existingData.StdOut -cne 'ABSENT') { throw 'Existing app data could not be safely checked. No data was removed.' }
        }
        Write-Host 'Recovery replaces the test app data with the retained backup. If signatures differ it also reinstalls the selected APK. Existing native data, when present, is backed up first.'
        if ((Read-Host 'Type RESTORE to restore this backup') -cne 'RESTORE') { Write-Host 'Cancelled. The backup is retained.'; return }
        $backup = Read-VerifiedBackup $backup.Folder
        if ((File-Sha256 $apk) -ine $expectedApkHash) { throw 'Recovery APK checksum changed. No app data was removed.' }
        if ($OriginalApk) { $installArguments = @('install', '-r', '-d', $apk) } else { $installArguments = @('install', '-r', $apk) }
        $result = Invoke-Adb $installArguments
        if (-not (Install-Succeeded $result)) {
            if (($result.StdOut + $result.StdErr) -notmatch 'INSTALL_FAILED_UPDATE_INCOMPATIBLE|INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES') {
                throw 'Recovery APK installation failed. No app data was removed. Your backup is retained.'
            }
            $script:RestoreMayBeNeeded = $true
            $result = Invoke-Adb @('uninstall', $script:Package)
            if (-not (Install-Succeeded $result)) { throw 'Recovery reinstall could not remove the app. Your backup is retained.' }
            $result = Invoke-Adb @('install', $apk)
            if (-not (Install-Succeeded $result)) { throw 'Recovery installation failed. The original APK and account backup are retained.' }
        }
        Stop-TestApp
        Require-DebugApp
        # Replace only this native app data tree, so interrupted copies cannot leave extra files.
        $script:RestoreMayBeNeeded = $true
        $null = Require-AdbSuccess @('shell', 'run-as', $script:Package, 'rm', '-rf', 'files/dados') 'Could not prepare the app data directory. Your backup is retained.'
        Restore-AdbArchive $backup.Archive
        Verify-InstalledData $backup
        Launch-TestApp
        return
    }
    $folder = New-VerifiedBackup
    $script:RetainedBackup = $folder
    if (-not $Transfer) { Write-Host 'Backup complete. The installed app and all its accounts are still on the device.'; return }
    $backup = Read-VerifiedBackup $folder
    $apk = Get-FixedApk
    $result = Invoke-Adb @('install', '-r', $apk)
    if (Install-Succeeded $result) {
        Verify-InstalledData $backup
        Launch-TestApp
        Write-Host 'Updated in place; no uninstall was needed.'
        return
    }
    $failure = $result.StdOut + $result.StdErr
    if ($failure -notmatch 'INSTALL_FAILED_UPDATE_INCOMPATIBLE|INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES') {
        throw 'APK installation failed for a reason other than signing. No app data was removed. Your backup is retained.'
    }
    Write-Host 'The installed test APK has the old temporary signature. Android requires one reinstall to switch to the stable signature.'
    Write-Host 'Your account data has been backed up and verified locally. UPDATE authorizes removing the old app, installing the fixed APK, and restoring that backup.'
    if ((Read-Host 'Type UPDATE to continue, or press Enter to keep the installed app') -cne 'UPDATE') { Write-Host 'Cancelled. The installed app and local backup are retained.'; return }
    $backup = Read-VerifiedBackup $folder
    if ((File-Sha256 $apk) -ine $script:ApkSha256) { throw 'APK checksum changed. No app data was removed.' }
    $script:RestoreMayBeNeeded = $true
    $result = Invoke-Adb @('uninstall', $script:Package)
    if (-not (Install-Succeeded $result)) { throw 'Uninstall did not succeed. Your verified backup is retained.' }
    $result = Invoke-Adb @('install', $apk)
    if (-not (Install-Succeeded $result)) { throw ('Installation failed. Keep the backup and rerun with -Restore -BackupFolder "' + $folder + '". Do not re-enter accounts.') }
    Stop-TestApp
    Require-DebugApp
    Restore-AdbArchive $backup.Archive
    Verify-InstalledData $backup
    Launch-TestApp
    Write-Host 'The stable test APK is installed with your backed-up accounts and settings. Future APKs signed with this same key can update it normally.'
}

# Dot-sourcing defines helpers for local testing; it never performs device operations.
if ($MyInvocation.InvocationName -ne '.') {
    try { Start-NuvioTransfer } catch {
        Write-Host ('Stopped: ' + $_.Exception.Message) -ForegroundColor Red
        Write-Host 'Keep the local backup folder. Do not clear app data or delete the backup.'
        if ($script:RestoreMayBeNeeded -and $script:RetainedBackup) {
            Write-Host 'To recover the retained backup, run:'
            $helperLiteral = "'" + $PSCommandPath.Replace("'", "''") + "'"
            $backupLiteral = "'" + $script:RetainedBackup.Replace("'", "''") + "'"
            Write-Host ('powershell.exe -NoProfile -ExecutionPolicy Bypass -File ' + $helperLiteral + ' -Restore -BackupFolder ' + $backupLiteral)
            Write-Host 'Add -OriginalApk to return to the original test APK with the same backed-up accounts.'
        }
        exit 1
    }
}
