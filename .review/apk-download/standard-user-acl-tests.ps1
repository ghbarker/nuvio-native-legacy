# Exercises only the helper's local Windows ACL functions. No ADB or phone access.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$HelperPath,
    [switch]$Child,
    [string]$ScratchRoot,
    [string]$ReportPath
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Read-AccessAcl([string]$Path) {
    $item = Get-Item -LiteralPath $Path -Force
    return $item.GetAccessControl([System.Security.AccessControl.AccessControlSections]::Access)
}

function Assert-PrivateAcl([string]$Path, [bool]$Inherited) {
    $acl = Read-AccessAcl $Path
    if (-not $Inherited) { Assert-True $acl.AreAccessRulesProtected 'Private root still inherits access rules.' }
    $rules = @($acl.GetAccessRules($true, $true, [System.Security.Principal.SecurityIdentifier]))
    $userSid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User.Value
    $expected = @($userSid, 'S-1-5-18', 'S-1-5-32-544') | Sort-Object
    Assert-True ($rules.Count -eq 3) 'Expected exactly three access rules.'
    $actual = @($rules | ForEach-Object { $_.IdentityReference.Value } | Sort-Object)
    Assert-True ((Compare-Object $expected $actual | Measure-Object).Count -eq 0) 'Unexpected principal has access.'
    foreach ($rule in $rules) {
        Assert-True ($rule.AccessControlType -eq [System.Security.AccessControl.AccessControlType]::Allow) 'Expected allow-only access rules.'
        Assert-True ($rule.FileSystemRights -eq [System.Security.AccessControl.FileSystemRights]::FullControl) 'Expected FullControl access.'
        Assert-True ($rule.IsInherited -eq $Inherited) 'Unexpected inherited/explicit rule state.'
        if (-not $Inherited) {
            $both = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit -bor [System.Security.AccessControl.InheritanceFlags]::ObjectInherit
            Assert-True ($rule.InheritanceFlags -eq $both) 'Root rules do not protect child files and directories.'
            Assert-True ($rule.PropagationFlags -eq [System.Security.AccessControl.PropagationFlags]::None) 'Unexpected root propagation flag.'
        }
    }
}

function Get-OwnerAndGroup([string]$Path) {
    $sections = [System.Security.AccessControl.AccessControlSections]::Owner -bor [System.Security.AccessControl.AccessControlSections]::Group
    $acl = (Get-Item -LiteralPath $Path).GetAccessControl($sections)
    return $acl.GetOwner([System.Security.Principal.SecurityIdentifier]).Value + '/' + $acl.GetGroup([System.Security.Principal.SecurityIdentifier]).Value
}

function Initialize-OriginalPrivateFolder {
    # Exact old ACL behavior, isolated in a scratch directory with no account data.
    $oldRoot = Join-Path $env:LOCALAPPDATA 'Nuvio-Test-Account-Transfer'
    if (-not (Test-Path -LiteralPath $oldRoot)) { $null = New-Item -ItemType Directory -Path $oldRoot }
    Reject-ReparsePoint $oldRoot
    $acl = New-Object System.Security.AccessControl.DirectorySecurity
    $acl.SetAccessRuleProtection($true, $false)
    $userSid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User
    $acl.SetOwner($userSid)
    foreach ($sid in @($userSid.Value, 'S-1-5-18', 'S-1-5-32-544')) {
        $identity = [System.Security.Principal.SecurityIdentifier]::new($sid)
        $rule = [System.Security.AccessControl.FileSystemAccessRule]::new($identity, 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')
        $acl.AddAccessRule($rule)
    }
    Set-Acl -LiteralPath $oldRoot -AclObject $acl
}

if ($Child) {
    $results = New-Object System.Collections.Generic.List[string]
    $originalOutcome = 'not run'
    $success = $false
    $failure = ''
    try {
        Assert-True ($PSVersionTable.PSVersion.Major -eq 5 -and $PSVersionTable.PSEdition -eq 'Desktop') 'Fixture must run in Windows PowerShell 5.1.'
        $identity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
        $principal = [System.Security.Principal.WindowsPrincipal]::new($identity)
        Assert-True (-not $principal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)) 'Fixture accidentally ran as an administrator.'
        $privileges = (& "$env:SystemRoot\System32\whoami.exe" /priv 2>&1 | Out-String)
        Assert-True ($LASTEXITCODE -eq 0) 'Could not inspect the test token privileges.'
        Assert-True ($privileges -notmatch 'SeSecurityPrivilege') 'Test token unexpectedly possesses SeSecurityPrivilege.'
        $results.Add('standard token: non-admin, no SeSecurityPrivilege')

        $tokens = $null
        $parseErrors = $null
        $ast = [System.Management.Automation.Language.Parser]::ParseFile($HelperPath, [ref]$tokens, [ref]$parseErrors)
        Assert-True (@($parseErrors).Count -eq 0) 'Helper has PowerShell syntax errors.'
        $wanted = @('Reject-ReparsePoint', 'Get-PrivateDirectoryAcl', 'Set-PrivateDirectoryAcl', 'Initialize-PrivateFolder')
        foreach ($name in $wanted) {
            $definitions = @($ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true) | Where-Object { $_.Name -eq $name })
            Assert-True ($definitions.Count -eq 1) ('Missing or duplicate helper function: ' + $name)
            . ([scriptblock]::Create($definitions[0].Extent.Text))
        }

        $env:LOCALAPPDATA = Join-Path $ScratchRoot 'new-root'
        $null = New-Item -ItemType Directory -Path $env:LOCALAPPDATA
        Initialize-PrivateFolder
        Assert-PrivateAcl $script:PrivateRoot $false
        $childFile = Join-Path $script:PrivateRoot 'non-secret-test.txt'
        Set-Content -LiteralPath $childFile -Value 'non-secret fixture' -Encoding ASCII
        Assert-PrivateAcl $childFile $true
        $childDirectory = Join-Path $script:PrivateRoot 'child-directory'
        $null = New-Item -ItemType Directory -Path $childDirectory
        Assert-PrivateAcl $childDirectory $true
        $before = (Read-AccessAcl $script:PrivateRoot).GetSecurityDescriptorSddlForm([System.Security.AccessControl.AccessControlSections]::Access)
        $identityBefore = Get-OwnerAndGroup $script:PrivateRoot
        Initialize-PrivateFolder
        Assert-PrivateAcl $script:PrivateRoot $false
        $after = (Read-AccessAcl $script:PrivateRoot).GetSecurityDescriptorSddlForm([System.Security.AccessControl.AccessControlSections]::Access)
        Assert-True ($before -ceq $after) 'Repeated initialization changed the protected access rules.'
        Assert-True ($identityBefore -ceq (Get-OwnerAndGroup $script:PrivateRoot)) 'Initialization changed owner or group.'
        $results.Add('new root: private DACL, child inheritance, idempotent, owner/group preserved')

        $env:LOCALAPPDATA = Join-Path $ScratchRoot 'existing-root'
        $root = Join-Path $env:LOCALAPPDATA 'Nuvio-Test-Account-Transfer'
        $null = New-Item -ItemType Directory -Path $root
        $acl = Read-AccessAcl $root
        $everyone = [System.Security.Principal.SecurityIdentifier]::new('S-1-1-0')
        $extra = [System.Security.AccessControl.FileSystemAccessRule]::new($everyone, 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')
        $acl.AddAccessRule($extra)
        (Get-Item -LiteralPath $root).SetAccessControl($acl)
        $fixtureFile = Join-Path $root 'preserved-non-secret.txt'
        Set-Content -LiteralPath $fixtureFile -Value 'unchanged fixture' -Encoding ASCII
        $fileHash = (Get-FileHash -LiteralPath $fixtureFile -Algorithm SHA256).Hash
        $identityBefore = Get-OwnerAndGroup $root
        Initialize-PrivateFolder
        Assert-PrivateAcl $root $false
        Assert-PrivateAcl $fixtureFile $true
        Assert-True ($fileHash -ceq (Get-FileHash -LiteralPath $fixtureFile -Algorithm SHA256).Hash) 'Existing file content changed.'
        Assert-True ($identityBefore -ceq (Get-OwnerAndGroup $root)) 'Existing root owner or group changed.'
        $results.Add('existing root: extra Everyone rule removed, content and owner/group preserved')

        $env:LOCALAPPDATA = Join-Path $ScratchRoot 'junction-root'
        $null = New-Item -ItemType Directory -Path $env:LOCALAPPDATA
        $target = Join-Path $ScratchRoot 'junction-target'
        $null = New-Item -ItemType Directory -Path $target
        $sentinel = Join-Path $target 'unchanged.txt'
        Set-Content -LiteralPath $sentinel -Value 'unchanged junction target' -Encoding ASCII
        $targetBefore = (Read-AccessAcl $target).GetSecurityDescriptorSddlForm([System.Security.AccessControl.AccessControlSections]::Access)
        $sentinelHash = (Get-FileHash -LiteralPath $sentinel -Algorithm SHA256).Hash
        $link = Join-Path $env:LOCALAPPDATA 'Nuvio-Test-Account-Transfer'
        $null = New-Item -ItemType Junction -Path $link -Target $target
        $rejected = $false
        try { Initialize-PrivateFolder } catch { $rejected = $_.Exception.Message -match 'link' }
        Assert-True $rejected 'Initialization did not reject the junction.'
        Assert-True ($targetBefore -ceq (Read-AccessAcl $target).GetSecurityDescriptorSddlForm([System.Security.AccessControl.AccessControlSections]::Access)) 'Junction target permissions changed.'
        Assert-True ($sentinelHash -ceq (Get-FileHash -LiteralPath $sentinel -Algorithm SHA256).Hash) 'Junction target content changed.'
        $results.Add('junction: rejected before touching target permissions or content')

        $env:LOCALAPPDATA = Join-Path $ScratchRoot 'original-behavior'
        $null = New-Item -ItemType Directory -Path $env:LOCALAPPDATA
        try {
            Initialize-OriginalPrivateFolder
            $originalOutcome = 'Original Set-Acl unexpectedly succeeded; positive fix checks remain valid.'
        } catch {
            if ($_.Exception.Message -match 'SeSecurityPrivilege') {
                $originalOutcome = 'Original Set-Acl reproduced SeSecurityPrivilege failure.'
            } else {
                $originalOutcome = 'Original Set-Acl failed with a different permission error: ' + $_.Exception.GetType().Name
            }
        }
        $success = $true
    } catch {
        $failure = $_.Exception.Message
    } finally {
        [pscustomobject]@{
            Success = $success
            Runtime = 'Windows PowerShell 5.1'
            Cases = @($results)
            OriginalBehavior = $originalOutcome
            Failure = $failure
            DeviceAccess = 'none'
        } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    }
    if (-not $success) { Write-Error $failure; exit 1 }
    exit 0
}

Assert-True ($env:OS -eq 'Windows_NT') 'This regression requires Windows.'
$fixtureRoot = Join-Path $env:PUBLIC ('nuvio-acl-fixture-' + [guid]::NewGuid().ToString('N'))
$userName = 'nvacl' + [guid]::NewGuid().ToString('N').Substring(0, 12)
$createdUser = $false
try {
    $random = New-Object byte[] 32
    $generator = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    try { $generator.GetBytes($random) } finally { $generator.Dispose() }
    $passwordText = 'NvA1!' + [Convert]::ToBase64String($random)
    $password = ConvertTo-SecureString $passwordText -AsPlainText -Force
    $passwordText = $null
    $user = New-LocalUser -Name $userName -Password $password -AccountNeverExpires -PasswordNeverExpires -Description 'Ephemeral non-admin ACL regression fixture'
    $createdUser = $true
    # Group names may be localized. The well-known SID identifies ordinary Users.
    $usersGroup = Get-LocalGroup -SID ([System.Security.Principal.SecurityIdentifier]::new('S-1-5-32-545'))
    Add-LocalGroupMember -Group $usersGroup.Name -Member $user
    $null = New-Item -ItemType Directory -Path $fixtureRoot
    $folder = Get-Item -LiteralPath $fixtureRoot
    $acl = $folder.GetAccessControl([System.Security.AccessControl.AccessControlSections]::Access)
    $acl.SetAccessRuleProtection($true, $false)
    foreach ($rule in $acl.GetAccessRules($true, $false, [System.Security.Principal.SecurityIdentifier])) { $acl.RemoveAccessRuleSpecific($rule) }
    foreach ($sid in @('S-1-5-18', 'S-1-5-32-544')) {
        $rule = [System.Security.AccessControl.FileSystemAccessRule]::new([System.Security.Principal.SecurityIdentifier]::new($sid), 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')
        $acl.AddAccessRule($rule)
    }
    $userRule = [System.Security.AccessControl.FileSystemAccessRule]::new($user.SID, 'Modify', 'ContainerInherit,ObjectInherit', 'None', 'Allow')
    $acl.AddAccessRule($userRule)
    $folder.SetAccessControl($acl)
    $fixtureScript = Join-Path $fixtureRoot 'fixture.ps1'
    $fixtureHelper = Join-Path $fixtureRoot 'helper.ps1'
    Copy-Item -LiteralPath $PSCommandPath -Destination $fixtureScript
    Copy-Item -LiteralPath $HelperPath -Destination $fixtureHelper
    $scratch = Join-Path $fixtureRoot 'scratch'
    $null = New-Item -ItemType Directory -Path $scratch
    $report = Join-Path $fixtureRoot 'result.json'
    $credential = [pscredential]::new(($env:COMPUTERNAME + '\' + $userName), $password)
    $arguments = '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $fixtureScript + '" -Child -HelperPath "' + $fixtureHelper + '" -ScratchRoot "' + $scratch + '" -ReportPath "' + $report + '"'
    $process = Start-Process -FilePath "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -Credential $credential -LoadUserProfile -WorkingDirectory $fixtureRoot -ArgumentList $arguments -Wait -PassThru
    $diagnosticDir = Join-Path (Get-Location).Path 'acl-diagnostics'
    $null = New-Item -ItemType Directory -Path $diagnosticDir -Force
    Assert-True (Test-Path -LiteralPath $report -PathType Leaf) 'Standard-user child did not produce a regression report.'
    Copy-Item -LiteralPath $report -Destination (Join-Path $diagnosticDir 'windows-acl-result.json')
    $result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    foreach ($case in $result.Cases) { Write-Host ('PASS: ' + $case) }
    Write-Host $result.OriginalBehavior
    Assert-True ($process.ExitCode -eq 0 -and $result.Success) ('Windows standard-user regression failed: ' + $result.Failure)
} finally {
    if ($createdUser) { Remove-LocalUser -Name $userName -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $fixtureRoot) {
        # Delete the junction itself before recursive cleanup; never recurse into its target.
        $junction = Join-Path $fixtureRoot 'scratch/junction-root/Nuvio-Test-Account-Transfer'
        if (Test-Path -LiteralPath $junction) { [System.IO.Directory]::Delete($junction) }
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
