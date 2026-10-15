# Windows PowerShell 5.1 or later. Run on your Windows computer, not the box.
Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$apkUrl = 'https://github.com/ghbarker/nuvio-native-legacy/releases/download/tv-mobile-test-logs-v18/Nuvio-TV-Mobile-Test-Logs18.apk'
$apkSha256 = '2c7d7155350b2a2db4202fa49229e56c250af09308a14a85e379aa68d5f2e0b7'
$toolsUrl = 'https://dl.google.com/android/repository/platform-tools-latest-windows.zip'
$script:AdbExecutable = $null

function Invoke-Adb {
    param([Parameter(Mandatory = $true)][string[]]$AdbArguments, [switch]$Interactive)
    $oldPreference = $ErrorActionPreference
    try {
        # Windows PowerShell can wrap native stderr in ErrorRecord objects.
        # Always check native completion explicitly after restoring this setting.
        $ErrorActionPreference = 'Continue'
        if ($Interactive) {
            & $script:AdbExecutable @AdbArguments
            $nativeOkay = $?
            $nativeExit = $LASTEXITCODE
        } else {
            $nativeOutput = & $script:AdbExecutable @AdbArguments 2>&1
            $nativeOkay = $?
            $nativeExit = $LASTEXITCODE
        }
    } finally {
        $ErrorActionPreference = $oldPreference
    }
    if (-not $nativeOkay -or $nativeExit -ne 0) {
        if (-not $Interactive -and $null -ne $nativeOutput) { Write-Host ([string]::Join("`n", [string[]]$nativeOutput)) }
        throw ('ADB stopped with exit code {0}. No uninstall or data clearing will be attempted.' -f $nativeExit)
    }
    if (-not $Interactive) {
        if ($null -eq $nativeOutput) { return '' }
        return ([string]::Join("`n", [string[]]$nativeOutput)).Trim()
    }
}

function Read-WirelessEndpoint {
    param([Parameter(Mandatory = $true)][string]$Prompt)
    $value = (Read-Host $Prompt).Trim()
    if ($value -notmatch '^(?<address>\[[^\]]+\]|[^:\s]+):(?<port>[0-9]{1,5})$') {
        throw 'Enter the IP address and port together, exactly as shown in Wireless debugging.'
    }
    $hostText = $Matches['address'].Trim([char[]]'[]')
    $portNumber = [int]$Matches['port']
    $parsedAddress = $null
    if (-not [Net.IPAddress]::TryParse($hostText, [ref]$parsedAddress) -or $portNumber -lt 1 -or $portNumber -gt 65535) {
        throw 'The IP address or port is invalid. Use the values on the Ugoos screen.'
    }
    return [PSCustomObject]@{ Target = $value; Address = $parsedAddress.ToString(); Port = $portNumber }
}

try {
    if ($env:OS -ne 'Windows_NT') { throw 'Run this helper in Windows PowerShell on your Windows computer.' }
    if ($PSVersionTable.PSVersion -lt [Version]'5.1') { throw 'Windows PowerShell 5.1 or later is required.' }
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'Windows LOCALAPPDATA is unavailable.' }
    $workDirectory = Join-Path $env:LOCALAPPDATA 'Nuvio-Ugoos-Update\Logs18'
    New-Item -ItemType Directory -Path $workDirectory -Force | Out-Null

    $adbCommand = Get-Command -Name 'adb.exe' -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $adbCommand) {
        $script:AdbExecutable = $adbCommand.Source
    } else {
        $toolsDirectory = Join-Path $workDirectory 'android-tools'
        $script:AdbExecutable = Join-Path $toolsDirectory 'platform-tools\adb.exe'
        if (-not (Test-Path -LiteralPath $script:AdbExecutable -PathType Leaf)) {
            Write-Host 'Downloading official Android platform-tools from Google...'
            $toolsArchive = Join-Path $workDirectory 'platform-tools-windows.zip'
            Invoke-WebRequest -UseBasicParsing -Uri $toolsUrl -OutFile $toolsArchive
            if ((Get-Item -LiteralPath $toolsArchive).Length -le 0) { throw 'The platform-tools download was empty.' }
            Expand-Archive -LiteralPath $toolsArchive -DestinationPath $toolsDirectory -Force
            if (-not (Test-Path -LiteralPath $script:AdbExecutable -PathType Leaf)) {
                throw 'The Google platform-tools archive did not contain platform-tools\adb.exe.'
            }
        }
    }
    $adbVersion = Invoke-Adb -AdbArguments @('version')
    if ($adbVersion -notmatch 'Android Debug Bridge version 1\.0\.(?<revision>[0-9]+)' -or [int]($Matches['revision']) -lt 41) {
        throw 'ADB is too old for wireless pairing. Install the current Google platform-tools and rerun this helper.'
    }
    $adbHelp = Invoke-Adb -AdbArguments @('help')
    if ($adbHelp -notmatch '(?m)^\s*pair\s+') {
        throw 'This ADB version does not support wireless pairing. Update the platform-tools on your Windows PATH and rerun.'
    }
    Write-Host ($adbVersion -split "`r?`n" | Select-Object -First 2)

    $apkPath = Join-Path $workDirectory 'Nuvio-TV-Mobile-Test-Logs18.apk'
    $needDownload = $true
    if (Test-Path -LiteralPath $apkPath -PathType Leaf) {
        $needDownload = (Get-FileHash -LiteralPath $apkPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne $apkSha256
    }
    if ($needDownload) {
        Write-Host 'Downloading the published Logs18 APK...'
        $partialPath = $apkPath + '.part'
        Invoke-WebRequest -UseBasicParsing -Uri $apkUrl -OutFile $partialPath
        $downloadHash = (Get-FileHash -LiteralPath $partialPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($downloadHash -cne $apkSha256) {
            throw ('APK checksum mismatch. Expected {0}; received {1}. Installation has been stopped.' -f $apkSha256, $downloadHash)
        }
        Move-Item -LiteralPath $partialPath -Destination $apkPath -Force
    }
    if ((Get-FileHash -LiteralPath $apkPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne $apkSha256) {
        throw 'The saved APK checksum does not match the approved Logs18 build. Installation has been stopped.'
    }
    Write-Host 'Logs18 APK checksum verified.'
    Invoke-Adb -AdbArguments @('start-server') -Interactive

    Write-Host ''
    Write-Host 'On the Ugoos: open Developer options > Wireless debugging > Pair device with pairing code.'
    $pairEndpoint = Read-WirelessEndpoint -Prompt 'PAIRING popup IP address:port'
    Write-Host 'ADB will now ask for the pairing code. Type the code directly into its prompt.'
    Invoke-Adb -AdbArguments @('pair', $pairEndpoint.Target) -Interactive

    Write-Host ''
    Write-Host 'Close the pairing popup. Use IP address & Port on the MAIN Wireless debugging screen.'
    Write-Host 'Its connection port must be different from the pairing popup port.'
    $connectEndpoint = Read-WirelessEndpoint -Prompt 'MAIN Wireless debugging IP address:port'
    if ($connectEndpoint.Address -cne $pairEndpoint.Address -or $connectEndpoint.Port -eq $pairEndpoint.Port) {
        throw 'Use the same Ugoos IP address and the DIFFERENT connection port from the main Wireless debugging screen.'
    }
    Invoke-Adb -AdbArguments @('connect', $connectEndpoint.Target) -Interactive
    $deviceState = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'get-state')
    if ($deviceState -cne 'device') { throw 'The selected device is not connected and authorized. Check the Ugoos screen.' }

    $model = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'shell', 'getprop', 'ro.product.model')
    $brand = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'shell', 'getprop', 'ro.product.brand')
    $manufacturer = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'shell', 'getprop', 'ro.product.manufacturer')
    $identity = $brand + ' ' + $manufacturer + ' ' + $model
    Write-Host ('Connected model: {0}; brand: {1}; manufacturer: {2}' -f $model, $brand, $manufacturer)
    if ($identity -notmatch '(?i)ugoos' -or $model -notmatch '(?i)(^|[^a-z0-9])am9(?:[ _-]*pro)?($|[^a-z0-9])') {
        throw 'This device could not be confirmed as a Ugoos AM9/AM9 Pro. No APK was installed.'
    }
    $abis = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'shell', 'getprop', 'ro.product.cpu.abilist')
    if ([string]::IsNullOrWhiteSpace($abis)) {
        $abis = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'shell', 'getprop', 'ro.product.cpu.abi')
    }
    if (($abis -split ',') -notcontains 'arm64-v8a' -and ($abis -split ',') -notcontains 'armeabi-v7a') {
        throw ('Expected an ARM Android device; reported ABIs: {0}. No APK was installed.' -f $abis)
    }
    Write-Host ('ARM ABI verified: {0}' -f $abis)
    if ((Get-FileHash -LiteralPath $apkPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne $apkSha256) {
        throw 'The APK changed after download verification. No installation was attempted.'
    }
    Write-Host 'Updating Nuvio TV/Mobile Test while keeping its accounts and settings...'
    $installResult = Invoke-Adb -AdbArguments @('-s', $connectEndpoint.Target, 'install', '-r', '-t', $apkPath)
    Write-Host $installResult
    if ($installResult -notmatch '(?m)^\s*Success\s*$') { throw 'ADB did not confirm a successful installation.' }
    Write-Host ''
    Write-Host 'Done. Open Nuvio TV/Mobile Test on the Ugoos.'
    Write-Host 'The official Nuvio app has a different package and can stay installed alongside it.'
} catch {
    Write-Host ''
    Write-Host ('Stopped: ' + $_.Exception.Message) -ForegroundColor Red
    Write-Host 'Keep the installed app. If you see a signature or version error, share that error before changing anything.'
    exit 1
}
