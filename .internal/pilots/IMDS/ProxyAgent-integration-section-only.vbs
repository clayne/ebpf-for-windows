'-------------------------------------------------------------------
'------------- EBPF Install/Uninstall Functions --------------------
'-------------------------------------------------------------------
' String constants
Const EBPF_EBPFCORE_DRIVER_NAME				= "eBPFCore"
Const EBPF_EXTENSION_DRIVER_NAME			= "NetEbpfExt"
Const EBPF_NETSH_EXTENSION_NAME				= "ebpfnetsh.dll"
Const EBPF_TRACING_STARTUP_TASK_NAME		= "eBpfTracingStartupTask"
Const EBPF_TRACING_STARTUP_TASK_FILENAME	= "ebpf_tracing_startup_task.xml"
Const EBPF_TRACING_PERIODIC_TASK_NAME		= "eBpfTracingPeriodicTask"
Const EBPF_TRACING_PERIODIC_TASK_FILENAME	= "ebpf_tracing_periodic_task.xml"
Const EBPF_TRACING_TASK_CMD					= "ebpf_tracing.cmd"

' Global variables
Dim WmiService : Set WmiService = GetObject("winmgmts:\\.\root\cimv2")
Dim g_tracingPath: g_tracingPath = FsObject.BuildPath(WshShell.ExpandEnvironmentStrings("%SystemRoot%"), "\Logs\eBPF")
Dim g_installPath: g_installPath = FsObject.BuildPath(WshShell.ExpandEnvironmentStrings("%ProgramFiles%"), "\ebpf-for-windows")


' Adds a new path to the System path, unless the path is already present in the system path.
Function AddSystemPath(pathToAdd)
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "AddSystemPath"

	AddSystemPath = True
	
	Dim sysENV : Set sysENV = WshShell.Environment("System")
	systemPath = sysENV("PATH")
	If InStr(systemPath, pathToAdd) = 0 Then
		sysENV("PATH") = systemPath + ";" + pathToAdd
	End If

	Set oTraceEvent = g_Trace.CreateEvent("INFO")		
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "pathToAdd", pathToAdd
	End With
	g_Trace.TraceEvent oTraceEvent
End Function

' Removes the given path from the System path, unless the path isn't present.
Function RemoveSystemPath(pathToRemove)
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "RemoveSystemPath"

	RemoveSystemPath =  True

	Dim sysENV : Set sysENV = WshShell.Environment("System")
	systemPath = sysENV("PATH")
	If InStr(systemPath, pathToRemove) <> 0 Then
		systemPath = Replace(systemPath, pathToRemove, "")
		systemPath = Replace(systemPath, ";;", ";")
		sysENV("PATH") = systemPath
	End If

	Set oTraceEvent = g_Trace.CreateEvent("INFO")		
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "pathToRemove", pathToRemove
	End With
	g_Trace.TraceEvent oTraceEvent
End Function

' This function moves the files from the source folder to the destination folder
Function MoveFilesToPath(sourcePath, destPath)
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "MoveFilesToPath"

	MoveFilesToPath = True	

	' Create the destination folder if it doesn't exist
	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "destPath", destPath
		If Not FsObject.FolderExists(destPath) Then		
			call FsObject.CreateFolder(destPath)
			If TraceError(g_Trace, "Failed to create folder " + destPath) = 0 Then
				MoveFilesToPath = False
			End If
		End If
	End With

	If MoveFilesToPath = True Then
		' Move all files and subfolders from the source folder to the destination folder
		Set oTraceEvent = g_Trace.CreateEvent("INFO")
		With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
			call FsObject.MoveFiles(sourcePath & "\*.*", destPath & "\", True)		
			If TraceError(g_Trace, "Failed to move files from " + sourcePath + " to " + destPath) <> 0 Then
				MoveFilesToPath = False
			End If

			call FsObject.MoveFolder(sourcePath & "\*", destPath & "\", True)
			If TraceError(g_Trace, "Failed to move subfolders from " + sourcePath + " to " + destPath) <> 0 Then
				MoveFilesToPath = False
			End If
		End With
	End If
End Function

' This function creates a scheduled task from a given XML file.
Function CreateScheduledTask(taskName, taskFile)
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "CreateScheduledTask"
	
	CreateScheduledTask = True
	
	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "taskName", taskName
		.setAttribute "taskFilePath", taskFilePath
	End With
	g_Trace.TraceEvent oTraceEvent

	' Create the scheduled task
	Dim taskFilePath : taskFilePath = FsObject.BuildPath(g_installPath, taskFile)
	if ExecuteAndTraceWithResults("schtasks.exe /create /f /tn " + taskName + " /xml """ + taskFilePath + """", g_trace).ExitCode <> 0 Then
		CreateScheduledTask = False
	End If
End Function

' This function creates tasks to start eBPF tracing at boot, and execute periodic rundowns
Function CreateEbpfTracingTasks()
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "CreateEbpfTracingTasks"

	CreateEbpfTracingTasks = true

	' Delete the tasks if they already exist
	call DeleteEbpfTracingTasks()

	' Create the scheduled tasks
	if CreateScheduledTask(EBPF_TRACING_STARTUP_TASK_NAME, EBPF_TRACING_STARTUP_TASK_FILENAME) = False Then
		CreateEbpfTracingTasks = False
	End If
	if CreateScheduledTask(EBPF_TRACING_PERIODIC_TASK_NAME, EBPF_TRACING_PERIODIC_TASK_FILENAME) = False Then
		call DeleteEbpfTracingTasks()
		CreateEbpfTracingTasks = False
	End If
End Function

' This function deletes the tasks to start eBPF tracing at boot, and execute periodic rundowns, and deletes the g_tracingPath
Function DeleteEbpfTracingTasks()
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "DeleteEbpfTracingTasks"

	DeleteEbpfTracingTasks = True

	' Execute the script to stop eBPF tracing
	Dim scriptPath : scriptPath = FsObject.BuildPath(g_installPath, EBPF_TRACING_TASK_CMD)
	If FsObject.FileExists(scriptPath) Then
		Set oTraceEvent = g_Trace.CreateEvent("INFO")
		With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
			.setAttribute "StopEbpfTracing", scriptPath
		End With
		g_Trace.TraceEvent oTraceEvent
		
		If ExecuteAndTraceWithResults("cmd.exe /c """"" + scriptPath + """ stop """ + g_tracingPath + """""", g_trace).ExitCode <> 0 Then
			DeleteEbpfTracingTasks = False
		End If
	Else
		Set oTraceEvent = g_Trace.CreateEvent("ERROR")
		With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
			.setAttribute "EbpfTaskScriptNotFound", scriptPath
		End With
		g_Trace.TraceEvent oTraceEvent
	End If

	' Delete the startup task. if it already exists
	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "DeleteEbpfTask", EBPF_TRACING_STARTUP_TASK_NAME
	End With
	g_Trace.TraceEvent oTraceEvent
	If ExecuteAndTraceWithResults("schtasks /delete /f /tn " + EBPF_TRACING_STARTUP_TASK_NAME, g_trace).ExitCode <> 0 Then
		DeleteEbpfTracingTasks = False
	End If

	' Delete the periodic task. if it already exists
	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "DeleteEbpfTask", EBPF_TRACING_PERIODIC_TASK_NAME
	End With
	g_Trace.TraceEvent oTraceEvent
	If ExecuteAndTraceWithResults("schtasks /delete /f /tn " + EBPF_TRACING_PERIODIC_TASK_NAME).ExitCode <> 0 Then
		DeleteEbpfTracingTasks = False
	End If
End Function

' This function checks if the given driver is already installed.
Function CheckDriverInstalled(driverName)
	On Error Resume Next

	CheckDriverInstalled = False

	Dim colServices, serviceQuery
	serviceQuery = "Select * from Win32_BaseService where Name='" + driverName + "'"
	Set colServices = WmiService.ExecQuery(serviceQuery)
	For Each objService in colServices
		CheckDriverInstalled = True
		Exit For
	Next
End Function

' This function installs the given kernel driver.
Function InstallEbpfDriver(driverName)
	On Error Resume Next
	Const THIS_FUNCTION_NAME = "InstallEbpfDriver"

	InstallEbpfDriver = True
	Dim driverFullPath: driverFullPath = FsObject.BuildPath(g_installPath, "\drivers\" + driverName + ".sys")
	
	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "driverFullPath", driverFullPath
	End With
	g_Trace.TraceEvent oTraceEvent

	if ExecuteAndTraceWithResults("sc.exe create " + driverName + " type=kernel start=auto binpath=""" + driverFullPath + """", g_trace).ExitCode <> 0
		InstallEbpfDriver = False
	End If
End Function

' This function stops then uninstalls the given kernel driver.
Function UninstallEbpfDriver(driverName)
	On Error Resume Next

	Const THIS_FUNCTION_NAME = "UninstallEbpfDriver"
	Dim colServices, serviceQuery, errReturnCode

	UninstallEbpfDriver = True
	Dim driverFullPath: driverFullPath = FsObject.BuildPath(g_installPath, "\drivers\" + driverName + ".sys")

	serviceQuery = "Select * from Win32_BaseService where Name='" + driverName + "'"
	Set colServices = WmiService.ExecQuery(serviceQuery)
	For Each objService in colServices

		Set oTraceEvent = g_Trace.CreateEvent("INFO")
		With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
			.setAttribute "driverFullPath", driverFullPath
		End With		
		g_Trace.TraceEvent oTraceEvent

		errReturnCode = objService.StopService()
		If errReturnCode <> 0 Then
			UninstallEbpfDriver = False
			Set oTraceEvent = g_Trace.CreateEvent("ERROR")
			With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
				.setAttribute "StopDriver", "False"
				.setAttribute "driverFullPath", driverFullPath
			End With		
			g_Trace.TraceEvent oTraceEvent
		End If

		errReturnCode = objService.Delete()
		If errReturnCode <> 0 Then
			UninstallEbpfDriver = False
			Set oTraceEvent = g_Trace.CreateEvent("ERROR")
			With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
				.setAttribute "Deleteriver", "False"
				.setAttribute "driverFullPath", driverFullPath
			End With		
			g_Trace.TraceEvent oTraceEvent
		End If
	Next
End Function

' This function installs eBPF for Windows on the machine and returns true successful, false otherwise
Function InstallEbpf(sourcePath)
	On Error Resume Next

	Const THIS_FUNCTION_NAME = "InstallEbpf"
	Dim eBpfCoreAlreadyInstalled, eBpfNetExtAlreadyInstalled
	Dim errReturnCode

	InstallEbpf = True

	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "g_installPath", g_installPath
	End With
	g_Trace.TraceEvent oTraceEvent

	' Check if eBPF for Windows is already installed
	eBpfCoreAlreadyInstalled = CheckDriverInstalled(EBPF_EBPFCORE_DRIVER_NAME)
	eBpfNetExtAlreadyInstalled = CheckDriverInstalled(EBPF_EXTENSION_DRIVER_NAME)
	If eBpfCoreAlreadyInstalled = True And eBpfNetExtAlreadyInstalled = True Then
		Set oTraceEvent = g_Trace.CreateEvent("INFO")
		With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
			.setAttribute "eBpfAlreadyInstalled", g_installPath
		End With
		g_Trace.TraceEvent oTraceEvent
		Exit Function
	Else
		If eBpfCoreAlreadyInstalled <> eBpfNetExtAlreadyInstalled Then
			InstallEbpf = False
			Set oTraceEvent = g_Trace.CreateEvent("ERROR")
			With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
				.setAttribute "eBpfCoreAlreadyInstalled", eBpfCoreAlreadyInstalled
				.setAttribute "eBpfNetExtAlreadyInstalled", eBpfNetExtAlreadyInstalled
			End With
			g_Trace.TraceEvent oTraceEvent
			Exit Function
		End If
	End If

	' Copy the files to the install path
	If Len(sourcePath) > 0 Then
		if Not MoveFilesToPath(sourcePath, g_installPath) Then
			InstallEbpf = False
			Exit Function
		End If
	End If

	' Install the drivers and add the install path to the system path
	If InstallEbpfDriver(EBPF_EBPFCORE_DRIVER_NAME) = False Then
		InstallEbpf = False
		Exit Function
	End If

	If InstallEbpfDriver(EBPF_EXTENSION_DRIVER_NAME) = False Then
		InstallEbpf = False
		call UninstallEbpfDriver(EBPF_EBPFCORE_DRIVER_NAME)
		Exit Function
	End If
	
	if AddSystemPath(g_installPath) = False Then
		InstallEbpf = False
	End If
End Function

' This function uninstalls eBPF for Windows on the machine and returns true successful, false otherwise
Function UninstallEbpf(shouldDeleteEbpfTracingTasks)
	On Error Resume Next

	Const THIS_FUNCTION_NAME = "UninstallEbpf"
	Dim errReturnCode

	UninstallEbpf = True

	if Not FsObject.FolderExists(g_installPath) Then
		Set oTraceEvent = g_Trace.CreateEvent("WARNING")
		With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
			.setAttribute "eBpfNotInstalled", g_installPath
		End With
		g_Trace.TraceEvent oTraceEvent
		UninstallEbpf = False
		' Not exiting: we still want to try to remove the drivers and tracing tasks, in case they were installed in a different location
	End If

	Set oTraceEvent = g_Trace.CreateEvent("INFO")
	With oTraceEvent.appendChild(oTraceEvent.ownerDocument.createElement(THIS_FUNCTION_NAME))
		.setAttribute "g_installPath", g_installPath
	End With
	g_Trace.TraceEvent oTraceEvent

	If UninstallEbpfDriver(EBPF_EBPFCORE_DRIVER_NAME) = False Then
		UninstallEbpf = False
	End If

	If UninstallEbpfDriver(EBPF_EXTENSION_DRIVER_NAME) = False Then
		UninstallEbpf = False
	End If

	If shouldDeleteEbpfTracingTasks Then
		if DeleteEbpfTracingTasks() = False Then
			UninstallEbpf = False
		End If
	End If

	if RemoveSystemPath(g_installPath) = False Then
		UninstallEbpf = False
	End If

	FsObject.DeleteFolder g_installPath, True
	If TraceError(g_Trace, "Failed to delete folder " + g_installPath) <> 0 Then
		UninstallEbpf = False
	End If
End Function