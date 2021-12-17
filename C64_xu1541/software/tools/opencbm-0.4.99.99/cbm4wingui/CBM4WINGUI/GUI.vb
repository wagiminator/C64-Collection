#Region "Copyright"
'-----------------
'GUI4CBM4WIN
'
' Copyright (C) 2004-2005 Leif Bloomquist
' Copyright (C) 2006      Wolfgang 0.4.0
' Copyright (C) 2006      Spiro Trikaliotis
' Copyright (C) 2006-2007 Payton Byrd
'
' This software Is provided 'as-is', without any express or implied
' warranty. In no event will the authors be held liable for any damages
' arising from the use of this software.
'
' Permission is granted to anyone to use this software for any purpose,
' including commercial applications, and to alter it and redistribute it
' freely, subject to the following restrictions:
'
'     1. The origin of this software must not be misrepresented; you must
'        not claim that you wrote the original software. If you use this
'        software in a product, an acknowledgment in the product
'        documentation would be appreciated but is not required.
'
'     2. Altered source versions must be plainly marked as such, and must
'        not be misrepresented as being the original software.
'
'     3. This notice may not be removed or altered from any source
'        distribution.
'

' Version 0.06 - First general release.
'         0.07 - Added quotes around parameters in DoCommand(), to handle paths with spaces.
'         0.08 - Added removal of .prg extensions with PC->64 copy.
'                Added file size calculation and display.
'                Added "Make Dir" button.
'                Prevented a mix of files and D64s from being selected.
'         0.09 - Fixed bug with ini file save location.
'                Uses cbmcopy '-f S' when writing files that end in .seq (and strips off extension)
'                Uses ",s" at end of name when reading files that end in .seq (and handles extension)
'                Option to inhibit the automatic directory reread after most operations, so errors aren't covered up.
'                Fixed hardcoded reference to c:\ drive in two places
'                Improved handling of default values when INI file has errors.
'
'   Additions by Wolfgang 0.4.0, based on Gui4Cbm4Win 0.0.9
'         0.40 - Renamed all "Warp"-Options into "No-Warp" ones
'                Added "auto" transfer option
'                Renamed the stdout and stderr log files to g4c4w*.log
'                Extended the "Detect" dialog in the options menu so that more than one drive is shown
'                Fixed the "status" action, the return string now needs to be taken from stdout
'                Fixed the "dir" action, the status string now needs to be taken from the last line on stdout
'                The "Morse" code action is now directed to the currently selected IEC bus device
'                Put the sources under the zlib/libpng license to clearly define usage and reusage
'                    by further developers, to make a crystal clear definition of this branch of
'                    Leif's software beeing OpenSource for now and all times without the copyleft
'                    restriction of the GNU General Public License, but still beeing compatible
'                    to the GPL and finally to allow distribution via Sourceforge. All in all I
'                    hope to preserve the will of Leif as much as possible with that license.
'                Added a "Reset bus" action button to the options dialog
'                Added a generalized check for error conditions in the commands executor
'                Format action: Added the switches "-v" and "-o" as well as the missing "-s" for printing the status
'                Fixed the format action, the status string now needs to be taken from the last line on stdout
'                Using cbmforng instead of cbmformat now
'                Initialize: Added querying the status after sending the "I0" command
'                Options dialog fix: the selected drive number is put to the main window object
'                Options dialog fix: the selected working directory patch is taken over to the main form
'                Temporary files are created within the Environ$("temp") (%TEMP%) specified directory
'                Added the quiet option "-q" to cbmcopy, so only error messages get printed
'                Added Chr$(34)/'"' quotations to all filename arguments given to DoCommand
'
'         0.41 - Fix the scratch command by appending a space character after the drive number
'                Added a "Validate" button
'TODO:
'                Improve the ASCII/PETSCII translation when reading dirs, for rename/scratch
'   
'         0.50 - Convert to VB.Net 2.0 - Payton Byrd
'                Make the user interface sizable
'
'         0.60 - Replaced calls to Shell with using the .Net Process object
'                Capturing the stdout and stderr via the process instead of reading files whenever possible.
'                Added Splitter Panel
'                Added Logger
'                Added CBM Drive selector to GUI.vb
#End Region

Option Strict Off
Option Explicit On

Imports VB = Microsoft.VisualBasic
Imports System.IO

Friend Class frmMain
    Inherits System.Windows.Forms.Form

    Private m_objResourceManager As Resources.ResourceManager = _
        New Resources.ResourceManager(Me.GetType().FullName, Me.GetType().Assembly)

    Private ReadOnly Property ResourceManager() As Resources.ResourceManager
        Get
            Return m_objResourceManager
        End Get
    End Property

    Private Sub About_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles About.Click
        MsgBox(String.Format(ResourceManager.GetString("ABOUT"), Application.ProductVersion), _
            MsgBoxStyle.Information, ResourceManager.GetString("ABOUT_TITLE"))
    End Sub

    'Called when user selects a file on the CBM directory

    Private Sub CBMDirectory_SelectedIndexChanged(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMDirectory.SelectedIndexChanged

        'Prevent "blocks free" from being selected
        If CBMDirectory.GetSelected(CBMDirectory.Items.Count - 1) Then
            CBMDirectory.SetSelected(CBMDirectory.Items.Count - 1, False)
        End If
    End Sub

    'Fetch the drive status strings.
    Private Sub CBMDriveStatus_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMDriveStatus.Click


        Dim Status As ReturnStringType

        Status = DoCommand("cbmctrl", "status " & DriveNumber, ResourceManager.GetString("READ_DRIVE_STATUS"))

        LastStatus.Text = UCase(Status.Output)
    End Sub

    'Format a floppy.
    Private Sub CBMFormat_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMFormat.Click
        Dim Result As Object

        Result = MsgBox(ResourceManager.GetString("FORMAT_MESSAGE"), _
            MsgBoxStyle.Exclamation Or MsgBoxStyle.YesNo, _
            ResourceManager.GetString("FORMAT_TITLE"))

        If Not (Result = MsgBoxResult.No) Then
            FormatDisk()
        End If
    End Sub

    Private Sub FormatDisk()
        Dim Status As ReturnStringType
        Dim args As String

        Prompt.Ask(ResourceManager.GetString("FORMAT_DISK_NAME"))

        If (Prompt.LastResult = CANCELSTRING) Then Exit Sub

        args = String.Format(" -vso {0} ""{1}""", DriveNumber, Prompt.LastResult.ToUpper())

        Status = DoCommand("cbmforng", args, ResourceManager.GetString("FORMATTING_DISK"))

        LastStatus.Text = UCase(Status.Output)

        System.Threading.Thread.Sleep(1000)

        RefreshCBMDir()
    End Sub

    Private Sub CBMInitialize_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMInitialize.Click


        DoCommand("cbmctrl", "command " & DriveNumber & " I0", ResourceManager.GetString("INITIALIZING_DRIVE"))

        CBMDriveStatus_Click(eventSender, eventArgs)
    End Sub

    Private Sub CBMRefresh_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMRefresh.Click


        Dim temp As String
        Dim Results As ReturnStringType

        Try

            'Probably the most complicated action - read a directory.
            CBMDirectory.Items.Clear()

            'Run the program
            Results = DoCommand("cbmctrl", "dir " & DriveNumber, ResourceManager.GetString("READING_DIRECTORY"), False)

            ' 'The drive status is always returned.
            ' LastStatus.Caption = UCase(Results.Errors)
            '
            ' It's not that simple anymore, we now need to extract it from the last line of stdout

            'Read in the complete output file -------------

            Dim reader As StringReader = New StringReader(Results.Output)
            Dim last As String = vbNullString

            'Check for empty file
            'First line is dir. name and ID
            temp = reader.ReadLine()
            CBMDiskName.Text = UCase(ExtractQuotes(temp))
            CBMDiskID.Text = UCase(VB.Right(temp, 5))

            temp = reader.ReadLine()
            While Not (temp Is Nothing)
                ' Whenever another line is available, store the current one into the panel
                CBMDirectory.Items.Add(temp.ToUpper()) 'Not only does uppercase look better, but Scratch, Rename need uppercase

                last = temp
                temp = reader.ReadLine()
            End While

            'The drive status is taken from the last line on stdout
            If Not (last Is Nothing) Then
                If Not last.Length = 0 Then
                    LastStatus.Text = last.ToUpper()
                End If
            End If

            reader.Close()

        Catch exception As Exception

            If Not (Err.Number = 53) Then
                MsgBox(exception.Message, MsgBoxStyle.Information, ResourceManager.GetString("REFRESH"))
            End If

            ClearCBMDir()

        End Try
    End Sub

    Private Sub CBMRename_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMRename.Click


        Dim T As Short
        Dim args As String

        For T = 0 To CBMDirectory.Items.Count - 1

            If (CBMDirectory.GetSelected(T)) Then

                Prompt.Reply.Text = ExtractQuotes(VB6.GetItemString(CBMDirectory, T))

                Prompt.Ask(String.Format(ResourceManager.GetString("ENTER_NAME"), _
                    ExtractQuotes(VB6.GetItemString(CBMDirectory, T))), False)

                If Not (Prompt.LastResult = CANCELSTRING) Then
                    args = String.Format("command {0} ""R0:{1}={2}""", _
                        DriveNumber, _
                        Prompt.LastResult.ToUpper(), _
                        ExtractQuotes(VB6.GetItemString(CBMDirectory, T)))

                    DoCommand("cbmctrl", args, ResourceManager.GetString("RENAMING"))
                Else
                    Exit Sub
                End If
            End If
        Next T

        RefreshCBMDir()
    End Sub

    Private Sub CBMReset_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMReset.Click

        DoCommand("cbmctrl", "reset", ResourceManager.GetString("RESETTING_DRIVES"))
    End Sub

    Private Sub CBMScratch_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMScratch.Click


        Dim T As Short
        Dim args As String
        Dim file As String

        For T = 0 To CBMDirectory.Items.Count - 1

            If (CBMDirectory.GetSelected(T)) Then

                file = ExtractQuotes(VB6.GetItemString(CBMDirectory, T))

                args = String.Format("command {0} ""S0:{1}""", _
                    DriveNumber, _
                    file)

                DoCommand("cbmctrl", args, _
                    String.Format(ResourceManager.GetString("SCRATCHING_FILE"), file))
            End If
        Next T

        RefreshCBMDir()
    End Sub

    Private Sub CBMValidate_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles CBMValidate.Click


        Dim args As String

        args = String.Format("command {0} ""V0:""", _
            DriveNumber)

        DoCommand("cbmctrl", args, ResourceManager.GetString("VALIDATING"))
    End Sub

    Private Sub CopyFromFloppy_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) _
      Handles CopyFromFloppy.Click

        Dim FilesSelected As Short
        Dim Result As DialogResult
        Dim FileName As String
        Dim FileNameOut As String
        Dim FileNameTemp As String
        Dim args As String

        FilesSelected = 0

        If Directory.Exists(CurrentDirectory) Then
            For Each strToCopy As String In CBMDirectory.SelectedItems
                FileName = ExtractQuotes(strToCopy).ToLower()

                'Check for SEQ file
                FileNameTemp = FileName.Trim() 'Remove spaces first

                If FileNameTemp.ToUpper().EndsWith("SEQ") Then
                    FileNameOut = FileName & ".seq"
                    FileName = FileName & ",s"
                Else
                    FileNameOut = FileName
                    'FileName stays the same
                End If

                FileNameOut = Path.Combine(CurrentDirectory, FileNameOut)

                Dim strTransfer As String = TransferString

                If strTransfer = "original" Then strTransfer = "auto"

                args = String.Format("--quiet --no-progress --transfer={0} -r {1} ""{2}"" --output=""{3}""", _
                    strTransfer, DriveNumber, FileName, FileNameOut)

                DoCommand("cbmcopy", args, String.Format(ResourceManager.GetString("COPYING_FROM_FLOPPY"), ExtractQuotes(strToCopy)))

                FilesSelected = FilesSelected + 1

                PCDirectory.Refresh()
            Next
        End If

        'No Files were selected, make a D64 instead.
        If (FilesSelected = 0) Then
            Result = MsgBox(ResourceManager.GetString("COPYING_NO_FILES_SELECTED"), _
                MsgBoxStyle.Question Or MsgBoxStyle.YesNo, _
                ResourceManager.GetString("COPYING_CREATE_D64"))

            If Not (Result = MsgBoxResult.No) Then

                Prompt.Ask(ResourceManager.GetString("ENTER_FILENAME"))
                If Not (Prompt.LastResult = CANCELSTRING) Then
                    Dim strTargetFilename As String = Prompt.LastResult
                    If Not strTargetFilename.ToLower().EndsWith(".d64") Then
                        strTargetFilename += ".d64"
                    End If
                    args = String.Format("--transfer={0} {1} {2} ""{3}""", _
                        TransferString, g_strNoWarp, DriveNumber, Path.Combine(CurrentDirectory, strTargetFilename))

                    DoCommand("d64copy", args, ResourceManager.GetString("CREATING_D64"))

                    PCDirectory.Refresh()
                End If
            End If
        End If
    End Sub

    Private Sub CopyToFloppy_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) _
      Handles CopyToFloppy.Click

        Dim T As Short
        Dim FilesSelected As Short
        Dim FileName As String
        Dim FileNameOut As String
        Dim SeqType As String
        Dim args As String

        FilesSelected = 0

        For T = 0 To PCDirectory.Items.Count - 1
            If (PCDirectory.GetSelected(T)) Then
                FilesSelected = FilesSelected + 1

                If PCDirectory.Items(T).ToUpper().EndsWith(".D64") Then 'Make Disk from D64
                    If MessageBox.Show("Do you want to format the disk first?", "Copy D64 to Disk", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2) = Windows.Forms.DialogResult.Yes Then
                        FormatDisk()
                    End If
                    WriteD64toFloppy(PCDirectory.Items(T))
                    Exit Sub 'Exit so only 1 D64 is copied!
                Else 'Copy a File
                    FileName = PCDirectory.Items(T).ToLower()

                    'Remove .prg extention
                    If FileName.EndsWith(".prg") Then
                        FileNameOut = FileName.Substring(0, FileName.Length - 4)
                    Else
                        FileNameOut = FileName
                    End If

                    'Change file type for .seq extension
                    If FileName.EndsWith(".seq") Then
                        SeqType = " --file-type S"
                        FileNameOut = FileName.Substring(0, FileName.Length - 4)
                    Else
                        SeqType = ""
                    End If

                    If FileNameOut.Length > 16 Then
                        FileNameOut = FileNameOut.Substring(0, 16)
                    End If

                    Dim strTransfer As String = TransferString

                    If strTransfer = "original" Then strTransfer = "auto"

                    args = String.Format("--quiet --no-progress --transfer={0} -w {1} ""{2}"" --output=""{3}""{4}", _
                        strTransfer, DriveNumber, Path.Combine(CurrentDirectory, FileName), FileNameOut, SeqType)

                    DoCommand("cbmcopy", args, _
                        String.Format(ResourceManager.GetString("COPYING_TO_FLOPPY"), PCDirectory.Items(T), UCase(FileNameOut)))
                End If
            End If
        Next T

        If (FilesSelected > 0) Then
            RefreshCBMDir()
        End If
    End Sub

    Private Sub WriteD64toFloppy(ByRef d64file As String)

        Dim Result As Object
        Dim args As String

        Result = MsgBox(ResourceManager.GetString("CONFIRM_D64_WRITE"), _
            MsgBoxStyle.Exclamation Or MsgBoxStyle.YesNo, _
            ResourceManager.GetString("CONFIRM_D64_WRITE_TITLE"))

        If Not (Result = MsgBoxResult.No) Then
            args = String.Format("--transfer={0} {1} ""{2}"" {3}", _
                TransferString, g_strNoWarp, Path.Combine(CurrentDirectory, d64file), DriveNumber)

            DoCommand("d64copy", args, ResourceManager.GetString("CREATE_DISK_FROM_D64"))

            CBMRefresh_Click(CBMRefresh, New System.EventArgs())

        End If
    End Sub

    ' Program Initialization
    Private Sub MainForm_Load(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles MyBase.Load

        On Error Resume Next

        Text = String.Format(ResourceManager.GetString("TITLE"), Application.ProductVersion)

        If (VB.Command() = "-leifdevelopment") Then
            'OptionsForm.PreviewCheck.CheckState = System.Windows.Forms.CheckState.Checked
        End If

        CheckExes()
        ExeDir = AddSlash(CurDir())

        LoadINI()
        frmConfiguration.LoadConfig()
        'DriveNumber = CShort(OptionsForm.DriveNum.Text)

        CurrentDirectory = frmConfiguration.InitialDirectory
        GotoDir(CurrentDirectory)
        PCRefresh_Click(PCRefresh, New System.EventArgs())

        Drive.Items.Clear()
        Drive.Items.Add(frmConfiguration.DefaultDeviceNumber)
        Drive.SelectedIndex = 0
    End Sub

    Public Sub SetCbmDrive()
        Drive.Items.Clear()
        For Each objRow As CbmDevices.CbmDevicesRow In frmConfiguration.m_objCbmDevices._CbmDevices.Rows
            Dim intIndex As Integer = Drive.Items.Add(objRow.Device)
            If (objRow._Default) Then
                Drive.SelectedIndex = intIndex
            End If
        Next
    End Sub

    'A slightly smarter version of Chdir, that handles drives as well.
    Private Sub GotoDir(ByRef FullPath As String)
        CurrentDirectory = FullPath
    End Sub


    'This function must be private, because of the return type.
    Private Function DoCommand( _
       ByRef Action As String, _
       ByRef Args As String, _
       ByRef WaitMessage As String, _
       Optional ByRef DeleteOutFile As Boolean = True) _
       As ReturnStringType

        DoCommand.Output = vbNullString
        DoCommand.Errors = vbNullString

        Static InProgress As Boolean
        Dim CmdLine As String
        Dim ErrorString As String = vbNullString
        Dim FirstLog As Boolean = True

        Try
            If (InProgress) Then
                MsgBox(ResourceManager.GetString("OPENCBM_IN_PROGRESS"), MsgBoxStyle.Critical)
                DoCommand.Errors = ResourceManager.GetString("OPENCBM_IN_PROGRESS_ERROR")
                DoCommand.Output = vbNullString
                Exit Function
            End If

            'Check command - mostly for debugging.
            Dim Result As Object
            If (frmConfiguration.PreviewCommands) Then
                Result = MsgBox(String.Format(ResourceManager.GetString("PREVIEW_MSG"), Action, Args), MsgBoxStyle.YesNo)

                If Result = MsgBoxResult.No Then
                    Exit Function
                End If
            End If

            'Flag that the background process is starting.
            InProgress = True

            'Show in Progress Dialog
            ' Waiting.Show
            Waiting.Label.Text = WaitMessage
            VB6.ShowForm(Waiting, VB6.FormShowConstants.Modeless, Me)

            CmdLine = String.Format("{0} {1}", Action, Args)

            Dim p As Process = New Process()

            p.StartInfo.FileName = Path.Combine(ExeDir, Action)
            p.StartInfo.Arguments = Args
            p.StartInfo.CreateNoWindow = True
            p.StartInfo.RedirectStandardError = True
            p.StartInfo.RedirectStandardOutput = True
            p.StartInfo.UseShellExecute = False

            If (p.Start()) Then

                Dim stdOutBuffer As New System.Text.StringBuilder
                Dim stdOut As StreamReader = p.StandardOutput
                Dim stdOutString As String
                Dim stdOutLength As Long = 0
                Dim stdErr As StreamReader = p.StandardError

                While Not p.HasExited
                    Application.DoEvents()
                    stdOutString = stdOut.ReadLine
                    stdOutBuffer.Append(stdOutString + vbNewLine)

                    If Not FirstLog Then
                        WriteLog(Nothing, stdOutString)
                    Else
                        WriteLog(CmdLine, stdOutString)
                        FirstLog = False
                    End If

                    'End If
                    Application.DoEvents()

                End While

                stdOutString = stdOut.ReadToEnd
                stdOutBuffer.Append(stdOutString)

                If Not FirstLog Then
                    WriteLog(Nothing, stdOutString)
                Else
                    WriteLog(CmdLine, stdOutString)
                    FirstLog = False
                End If

                Log.AppendText("========================================" & vbNewLine)

                DoCommand.Output = stdOutBuffer.ToString()
                DoCommand.Errors = stdErr.ReadToEnd()

            End If
        Catch exception As Exception

            DoCommand.Errors = exception.ToString()

        Finally

            Waiting.Hide()
            InProgress = False

            If (Not DoCommand.Errors = "") Then
                MsgBox(DoCommand.Errors, MsgBoxStyle.OkOnly, Action)
            End If

        End Try
    End Function

    Private Sub WriteLog(ByVal CmdLine As String, ByVal outString As String)
        If Not (CmdLine Is Nothing) Then
            If CmdLine.Length > 0 Then
                Log.AppendText(String.Format("{0:yyyy/MM/dd HH:mm:ss} {1}", DateTime.Now, CmdLine) & vbNewLine)
            End If
        End If

        Log.AppendText(outString & vbNewLine)
        Log.Select(Log.Text.Length - 1, 0)
        Log.ScrollToCaret()
        Log.Refresh()


    End Sub

    Private Sub MakeDir_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles MakeDir.Click
        Prompt.Ask(ResourceManager.GetString("ENTER_DIRECTORY_NAME"))
        If (Prompt.LastResult = CANCELSTRING) Then Exit Sub

        MkDir(AddSlash(CurrentDirectory) & Prompt.LastResult)
        PCRefresh_Click(PCRefresh, New System.EventArgs())
    End Sub

    Private Sub Options_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles Options.Click

        If frmConfiguration.ShowDialog(Me) = Windows.Forms.DialogResult.OK Then
            Me.CurrentDirectory = frmConfiguration.InitialDirectory
            GotoDir(CurrentDirectory)
        End If

    End Sub

    Private Sub PCDelete_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles PCDelete.Click
        Dim Result As Object

        Result = MsgBox(ResourceManager.GetString("DELETE_SELECTED_FILES"), _
            MsgBoxStyle.YesNo Or MsgBoxStyle.Question, _
            ResourceManager.GetString("DELETE_SELECTED_FILES_TITLE"))
        If (Result = MsgBoxResult.No) Then Exit Sub

        Dim objList As New ArrayList

        For Each strToDelete As String In PCDirectory.SelectedItems
            objList.Add(Path.Combine(CurrentDirectory, strToDelete))
        Next

        For Each strToDelete As String In objList
            File.Delete(strToDelete)
        Next

        PCDirectory.Refresh()
    End Sub

    Private m_blnBusy As Boolean

    Private Sub PCRefresh_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles PCRefresh.Click
        PCDirectory.Refresh()
    End Sub

    Private Sub PCRename_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles PCRename.Click
        Dim T As Short

        For T = 0 To PCDirectory.Items.Count - 1
            Dim strFileName As String = PCDirectory.Items(T)
            If (PCDirectory.GetSelected(T)) Then
                Prompt.Reply.Text = strFileName
                Prompt.Ask(String.Format(ResourceManager.GetString("ENTER_NAME"), strFileName), False)

                If Not (Prompt.LastResult = CANCELSTRING) And _
                  File.Exists(Path.Combine(CurrentDirectory, strFileName)) Then
                    File.Move(Path.Combine(CurrentDirectory, strFileName), _
                        Path.Combine(CurrentDirectory, Prompt.LastResult))
                End If
            End If
        Next T

        PCDirectory.Refresh()
    End Sub

    Private Function ExtractQuotes(ByRef FullString As String) As String
        Dim Quote1 As Short
        Dim Quote2 As Short

        ExtractQuotes = FullString

        Try

            Quote1 = InStr(FullString, Chr(34))
            Quote2 = InStr(Quote1 + 1, FullString, Chr(34))
            If Quote1 < Quote2 And Quote1 > -1 And Quote2 > -1 Then
                ExtractQuotes = Mid(FullString, Quote1 + 1, Quote2 - Quote1 - 1)
            Else
                Me.WriteLog(Nothing, String.Format("FullString: [{0}]", FullString))

            End If

        Catch exception As Exception

            MsgBox(exception.ToString())

        End Try
    End Function

    Private Sub MainForm_FormClosing(ByVal eventSender As System.Object, ByVal eventArgs As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing
        Dim Cancel As Boolean = eventArgs.Cancel
        Dim UnloadMode As System.Windows.Forms.CloseReason = eventArgs.CloseReason
        'Make sure all child forms are closed
        End
        eventArgs.Cancel = Cancel
    End Sub

    Private Sub RunFile_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles RunFile.Click
        Dim T As Short
        Dim hWnd As Object = Nothing

        For T = 0 To PCDirectory.Items.Count - 1
            If (PCDirectory.GetSelected(T)) Then
                ShellExecute(hWnd, "open", PCDirectory.Items(T), vbNullString, CurrentDirectory, 1)

                'Stop so only the first file is executed
                Exit Sub
            End If
        Next T
    End Sub

    Public Function PubDoCommand( _
      ByRef Action As String, _
      ByRef Args As String, _
      ByRef WaitMessage As String, _
      Optional ByRef DeleteOutFile As Boolean = True) As String

        Dim Returns As ReturnStringType

        Returns = DoCommand(Action, Args, WaitMessage, DeleteOutFile)

        PubDoCommand = Returns.Output
    End Function

    'Check that the program is in the correct place, berate user if not :-)
    Private Sub CheckExes()
        Dim FilesOK As Boolean

        FilesOK = Not (Dir("cbmctrl.exe") = "")

        If Not (FilesOK) Then

            MsgBox(String.Format(ResourceManager.GetString("OPENCBM_NOT_FOUND"), CurDir()), _
                MsgBoxStyle.Critical, ResourceManager.GetString("ERROR"))

            End

        End If
    End Sub

    'Refresh the directory - only if automatic refresh hasn't been turned off
    Private Sub RefreshCBMDir()
        If (frmConfiguration.RefreshAfter) Then
            CBMRefresh_Click(CBMRefresh, New System.EventArgs())
        Else
            ClearCBMDir()
        End If
    End Sub

    'Clear the directory listing, because contents have changed
    Private Sub ClearCBMDir()
        CBMDirectory.Items.Clear()
        CBMDiskName.Text = ""
        CBMDiskID.Text = ""
        LastStatus.Text = ""
    End Sub

    Private Sub Drive_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Drive.SelectedIndexChanged
        DriveNumber = CShort(Drive.SelectedItem)
    End Sub

    Private Sub cmdBrowse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cmdBrowse.Click
        folderBrowser.SelectedPath = CurrentDirectory
        If folderBrowser.ShowDialog() = Windows.Forms.DialogResult.OK Then
            CurrentDirectory = folderBrowser.SelectedPath
        End If
    End Sub

    Private Property CurrentDirectoryPath() As String
        Get
            Return PCDirectory.SelectedPath
        End Get
        Set(ByVal value As String)
            If (Directory.Exists(value)) Then
                PCDirectory.SelectedPath = value
                PCWorkingDir.Text = value
            End If
        End Set
    End Property

    Private Property CurrentDirectory() As String
        Get
            Return PCWorkingDir.Text
        End Get
        Set(ByVal value As String)
            If (Directory.Exists(value)) Then
                PCWorkingDir.Text = value

                If PCDirectory.SelectedPath <> value Then
                    PCDirectory.SelectedPath = value
                End If

            Else
                CurrentDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)
            End If
        End Set
    End Property

    Private Sub PCDirectory_FileSelected(ByVal sender As System.Object, _
      ByVal fse As FilesBrowser.FileSelectEventArgs) Handles PCDirectory.FileSelected
        If Not m_blnBusy Then

            Dim lngBytes As Long
            Dim shtD64Selected As Short

            m_blnBusy = True

            shtD64Selected = -1
            lngBytes = 0

            'Refresh the KB/Blocks display
            For Each item As String In PCDirectory.SelectedItems
                Dim filename As String = Path.Combine(CurrentDirectory, item)

                ' Check if file exists first
                If Not File.Exists(filename) Then
                    ' File does not exist anymore
                    PCDirectory.ClearSelected()
                    lngBytes = 0
                    PCDirectory.Refresh()
                    Exit For
                Else

                    lngBytes += FileLen(filename)

                End If
            Next

            KBText.Text = String.Format(ResourceManager.GetString("KB_FORMAT"), lngBytes / 1024)

            Dim lngBlocks As Long
            lngBlocks = lngBytes / 254

            If lngBytes Mod 254 > 0 Then
                lngBlocks += 1
            End If

            BlockText.Text = String.Format(ResourceManager.GetString("BLOCKS_FORMAT"), lngBlocks)

        End If
        m_blnBusy = False

    End Sub

    Private Sub PCWorkingDir_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles PCWorkingDir.TextChanged

    End Sub

    Private Sub PCDirectory_SelectedPathChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles PCDirectory.SelectedPathChanged
        If CurrentDirectory <> PCDirectory.SelectedPath Then
            CurrentDirectory = PCDirectory.SelectedPath
        End If
    End Sub
End Class