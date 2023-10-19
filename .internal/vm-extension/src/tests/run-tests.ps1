# Change the working directory to the root of the test environment, so to simulate the actual env that will be set up by the VM Agent.
Set-Variable -Name "testRootFolder" -Value "C:\_ebpf\vm_ext\tests"
$currentDirectory = Get-Location
Set-Location "$testRootFolder"

# Dot source the utility script
. ..\scripts\common.ps1

$testPass = 0

function Exit-Tests {
    param (
        [int]$testPass
    )

    Set-Location $currentDirectory

    if ($testPass -eq 0) {
        Write-Log -level $LogLevelInfo -message "Tests successfully PASSED."
    } else {
        Write-Log -level $LogLevelError -message "Tests FAILED."
    }

    exit $testPass
}

function DownloadAndUnpackEbpfRedistPackage {
    param (
        [string]$packageVersion,
        [string]$targetDirectory
    )

    Write-Log -level $LogLevelInfo -message "DownloadAndUnpackEbpfRedistPackage($packageVersion, $targetDirectory)"

    $res = 0
    try {
        # Download the eBPF redist package from the MS CodeHub feed, and unpack just the eBPF package to the target directory
        $nugetArgs = @{
            FilePath = 'nuget.exe'
            ArgumentList = "install eBPF-for-Windows-Redist -version $packageVersion -Source https://mscodehub.pkgs.visualstudio.com/eBPFForWindows/_packaging/eBPFForWindows/nuget/v3/index.json -OutputDirectory $targetDirectory"
            Wait = $true
        }
        Start-Process @nugetArgs | Out-Null

        # Unpack the eBPF package to the target directory
        Rename-Item -Path "$targetDirectory\eBPF-for-Windows-Redist.$packageVersion\eBPF-for-Windows-Redist.$packageVersion.nupkg" -NewName "eBPF-for-Windows-Redist.$packageVersion.nupkg.zip" | Out-Null
        Expand-Archive -Path "$targetDirectory\eBPF-for-Windows-Redist.$packageVersion\eBPF-for-Windows-Redist.$packageVersion.nupkg.zip" -DestinationPath "$targetDirectory\eBPF-for-Windows-Redist.$packageVersion\temp" | Out-Null

        # Copy the eBPF package to the target directory (in a "bin" subfolder), and remove the temp folder.
        Copy-Item -Path "$targetDirectory\eBPF-for-Windows-Redist.$packageVersion\temp\package\bin" -Destination "$targetDirectory\v$packageVersion\bin" -Recurse -Force | Out-Null
        Remove-Item -Path "$targetDirectory\eBPF-for-Windows-Redist.$packageVersion" -Recurse -Force | Out-Null
    }
    catch {
        $res = 1
        Write-Log -level $LogLevelError -message "An error occurred: $_"
    }

    return $res
}

function Setup-Test-Package {
    param (
        [string]$packageVersion,
        [string]$testRedistTargetDirectory
    )

    Write-Log -level $LogLevelInfo -message "Setup-Test-Package($packageVersion, $testRedistTargetDirectory)"

    $res = 0
    if ((Delete-Directory -destinationPath "$EbpfPackagePath") -ne 0) {
        $res = 1
    }    
    if ((Copy-Directory -sourcePath "$testRedistTargetDirectory\v$packageVersion" -destinationPath "$EbpfPackagePath") -ne 0) {
        $res = 1
    }

    return $res
}

# Load test-environment (current working folder is the root folder in which the entire ZIP in unzipped).
if ((Get-HandlerEnvironment -handlerEnvironmentFullPath "$DefaultHandlerEnvironmentFilePath") -eq 0) {
    
    # Override the default package path.
    $EbpfPackagePath = ".\package"

    # Mock the VM-Agent provisioning the SequenceNumber environment variable.
    Set-Item -Path Env:\$VmAgentEnvVar_SEQUENCE_NO -Value "1" | Out-Null

    # Test cases
    #######################################################
    # Raw environment cleanup.    
    $null = Remove-Item -Path "$global:LogFilePath" -Recurse -Force 2>&1
    # Re-run just to log the HandlerEnvironment contents (in path not available before reading it)
    Get-HandlerEnvironment -handlerEnvironmentFullPath "$DefaultHandlerEnvironmentFilePath"
    Write-Log -level $LogLevelInfo -message "= Cleaning up environment =================================================================================================="        
    $null = net stop eBPFCore 2>&1
    $null = sc.exe delete eBPFCore 2>&1
    $null = net stop NetEbpfExt 2>&1
    $null = sc.exe delete NetEbpfExt 2>&1
    $null = netsh delete helper ebpfnetsh.dll 2>&1
    $null = Remove-DirectoryFromSystemPath "$EbpfDefaultInstallPath" 2>&1
    $null = Remove-Item -Path "$EbpfDefaultInstallPath" -Recurse -Force 2>&1

    # Clean-up and set up the test environment with two versions of the eBPF redist package.
    $testRedistTargetDirectory = ".\_ebpf-redist"
    Delete-Directory -destinationPath $testRedistTargetDirectory | Out-Null
    $res = DownloadAndUnpackEbpfRedistPackage -packageVersion "0.9.1" -targetDirectory $testRedistTargetDirectory
    if ($res -ne 0) {
        Exit-Tests -testPass 1
    }
    $res = DownloadAndUnpackEbpfRedistPackage -packageVersion "0.11.0.2" -targetDirectory $testRedistTargetDirectory
    if ($res -ne 0) { 
        Exit-Tests -testPass 1
    }

    # Spcific test cases regarding eBPF-only updates.
    $currProductVersion = "0.9.0"
    $newProductVersion = "0.10.0"
    $comparison = Compare-VersionNumbers -version1 $currProductVersion -version2 $newProductVersion
    Write-Log -level $LogLevelInfo -message "(v$currProductVersion) Vs (v$newProductVersion) -> $comparison"
    if ($comparison -ne -1) {
        Exit-Tests -testPass 1
    }
    $currProductVersion = "0.10.0"
    $newProductVersion = "0.10.0"
    $comparison = Compare-VersionNumbers -version1 $currProductVersion -version2 $newProductVersion
    Write-Log -level $LogLevelInfo -message "(v$currProductVersion) Vs (v$newProductVersion) -> $comparison"
    if ($comparison -ne 0) {
        Exit-Tests -testPass 1
    }
    $currProductVersion = "0.10.0"
    $newProductVersion = "0.9.0"
    $comparison = Compare-VersionNumbers -version1 $currProductVersion -version2 $newProductVersion
    Write-Log -level $LogLevelInfo -message "(v$currProductVersion) Vs (v$newProductVersion) -> $comparison"
    if ($comparison -ne 1) {
        Exit-Tests -testPass 1
    }
    
    # Spcific test cases regarding hanler-only updates.
    $currProductVersion = "0.9.0"
    $newProductVersion = "0.9.0.1"
    $comparison = Compare-VersionNumbers -version1 $currProductVersion -version2 $newProductVersion
    Write-Log -level $LogLevelInfo -message "(v$currProductVersion) Vs (v$newProductVersion) -> $comparison"
    if ($comparison -ne 2) {
        Exit-Tests -testPass 1
    }
    $currProductVersion = "0.10.0"
    $newProductVersion = "0.9.0.1"
    $comparison = Compare-VersionNumbers -version1 $currProductVersion -version2 $newProductVersion
    Write-Log -level $LogLevelInfo -message "(v$currProductVersion) Vs (v$newProductVersion) -> $comparison"
    if ($comparison -ne 1) {
        Exit-Tests -testPass 1
    }
    $currProductVersion = "0.9.0"
    $newProductVersion = "0.10.0.1"
    $comparison = Compare-VersionNumbers -version1 $currProductVersion -version2 $newProductVersion
    Write-Log -level $LogLevelInfo -message "(v$currProductVersion) Vs (v$newProductVersion) -> $comparison"
    if ($comparison -ne -1) {
        Exit-Tests -testPass 1
    }

    # Test that the status file name has the right sequence number ($EbpfExtensionName.1002.settings is artificially set to the one modified last).
    Report-Status -name $StatusName -operation "test" -status $StatusTransitioning -statusCode 0 -$statusMessage "Dummy status"
    $statusFileName = Get-ChildItem -Path "$($global:eBPFHandlerEnvObj.handlerEnvironment.statusFolder)" -Filter "*.status" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($statusFileName.Name -ne "1.status") {
        Exit-Tests -testPass 1
        Write-Log -level $LogLevelError -message "Status file name is not correct: $statusFileName"
    } else {
        Write-Log -level $LogLevelInfo -message "Status file name is correct: $statusFileName"
    }

    # Install an old version, i.e. Add a new handler on the VM (Install and Enable)
    Write-Log -level $LogLevelInfo -message "= Install an old version =================================================================================================="
    if ((Setup-Test-Package -packageVersion "0.9.1" -testRedistTargetDirectory $testRedistTargetDirectory) -ne 0) {
        Exit-Tests -testPass 1
    }
    if ((Install-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    } 
    if ((Enable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
        
    # Simulate a handler-only update, by changing the handler's new target version in the VERSION environment variable.
    Write-Log -level $LogLevelInfo -message "= Simulate a handler-only update =========================================================================================="
    if ((Disable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    if ((Update-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) { # Always NOP on update
        Exit-Tests -testPass 1
    }
    if ((Install-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    if ((Enable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }

    # Update to a newer version, i.e. handler's update is called (Disable and Update).
    Write-Log -level $LogLevelInfo -message "= Update to newer version ================================================================================================="
    if ((Setup-Test-Package -packageVersion "0.11.0.2" -testRedistTargetDirectory $testRedistTargetDirectory) -ne 0) {
        Exit-Tests -testPass 1
    }
    if ((Disable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    if ((Update-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) { # Always NOP on update
        Exit-Tests -testPass 1
    }
    if ((Install-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    if ((Enable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    
    # Downgrade to an older version
    Write-Log -level $LogLevelInfo -message "= Downgrade to an older version =============================================================================="
    if ((Setup-Test-Package -packageVersion "0.9.1" -testRedistTargetDirectory $testRedistTargetDirectory) -ne 0) {
        Exit-Tests -testPass 1
    }
    # if ((Set-EnvironmentVariable -variableName $VmAgentEnvVar_VERSION -variableValue "0.9.0.0") -ne 0) {
    #     Exit-Tests -testPass 1
    # }
    if ((Disable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    if ((Update-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) { # Always NOP on update
        Exit-Tests -testPass 1
    }
    if ((Install-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    if ((Enable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
    
    # Uninstall, i.e. Remove a handler from the VM (Disable and Uninstall): https://github.com/Azure/azure-vmextension-publishing/wiki/2.0-Partner-Guide-Handler-Design-Details#222-remove-a-handler-from-the-vm-disable-and-uninstall
    Write-Log -level $LogLevelInfo -message "= Uninstall ==============================================================================================================="
    if ((Disable-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) { # The VM Agent will first call 'Disable' on the handler
        Exit-Tests -testPass 1
    }  
    if ((Uninstall-eBPF-Handler) -ne $EbpfStatusCode_SUCCESS) {
        Exit-Tests -testPass 1
    }
} else {
    Exit-Tests -testPass 1
    Write-Log -level $LogLevelError -message "Failed to load '$DefaultHandlerEnvironmentFilePath'."
}

Exit-Tests -testPass 0
