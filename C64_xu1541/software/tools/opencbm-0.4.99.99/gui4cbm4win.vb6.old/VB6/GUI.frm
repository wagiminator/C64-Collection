VERSION 5.00
Begin VB.Form MainForm 
   Caption         =   "GUI4CBM4WIN"
   ClientHeight    =   6975
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10965
   Icon            =   "GUI.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   6975
   ScaleWidth      =   10965
   StartUpPosition =   3  'Windows Default
   Begin VB.DriveListBox Drive1 
      Height          =   315
      Left            =   240
      TabIndex        =   26
      Top             =   360
      Width           =   3375
   End
   Begin VB.Frame CBMDrive 
      Caption         =   "Commodore Drive"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   6735
      Left            =   5760
      TabIndex        =   1
      Top             =   120
      Width           =   5175
      Begin VB.CommandButton CBMValidate 
         Caption         =   "Validate"
         Height          =   375
         Left            =   4080
         TabIndex        =   36
         Top             =   2280
         Width           =   975
      End
      Begin VB.CommandButton CBMInitialize 
         Caption         =   "Initialize"
         Height          =   375
         Left            =   4080
         TabIndex        =   25
         Top             =   1800
         Width           =   975
      End
      Begin VB.CommandButton CBMFormat 
         Caption         =   "Format"
         Height          =   375
         Left            =   4080
         TabIndex        =   10
         Top             =   1320
         Width           =   975
      End
      Begin VB.ListBox CBMDirectory 
         BackColor       =   &H00C00000&
         BeginProperty Font 
            Name            =   "Fixedsys"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00FF8383&
         Height          =   5235
         ItemData        =   "GUI.frx":0442
         Left            =   120
         List            =   "GUI.frx":0444
         MultiSelect     =   2  'Extended
         TabIndex        =   9
         Top             =   600
         Width           =   3855
      End
      Begin VB.CommandButton CBMReset 
         Caption         =   "Reset"
         Height          =   375
         Left            =   4080
         TabIndex        =   8
         Top             =   5160
         Width           =   975
      End
      Begin VB.CommandButton CBMDriveStatus 
         Caption         =   "Status"
         Height          =   375
         Left            =   4080
         TabIndex        =   7
         Top             =   5640
         Width           =   975
      End
      Begin VB.CommandButton CBMRefresh 
         Caption         =   "Directory"
         Height          =   375
         Left            =   4080
         TabIndex        =   6
         Top             =   840
         Width           =   975
      End
      Begin VB.CommandButton CBMRename 
         Caption         =   "Rename"
         Height          =   375
         Left            =   4080
         TabIndex        =   5
         Top             =   3240
         Width           =   975
      End
      Begin VB.CommandButton CBMScratch 
         Caption         =   "Scratch"
         Height          =   375
         Left            =   4080
         TabIndex        =   4
         Top             =   3720
         Width           =   975
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Last Drive Status:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   34
         Top             =   6120
         Width           =   2175
      End
      Begin VB.Label LastStatus 
         BackColor       =   &H00C00000&
         BorderStyle     =   1  'Fixed Single
         BeginProperty Font 
            Name            =   "Fixedsys"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00FF8383&
         Height          =   285
         Left            =   120
         TabIndex        =   33
         Top             =   6360
         Width           =   4935
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Drive:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   5
         Left            =   4080
         TabIndex        =   28
         Top             =   4920
         Width           =   2175
      End
      Begin VB.Label CBMDiskName 
         BackColor       =   &H00FF8383&
         BorderStyle     =   1  'Fixed Single
         BeginProperty Font 
            Name            =   "Fixedsys"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C00000&
         Height          =   285
         Left            =   120
         TabIndex        =   14
         Top             =   240
         Width           =   2775
         WordWrap        =   -1  'True
      End
      Begin VB.Label CBMDiskID 
         Alignment       =   2  'Center
         BackColor       =   &H00FF8383&
         BorderStyle     =   1  'Fixed Single
         BeginProperty Font 
            Name            =   "Fixedsys"
            Size            =   9
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00C00000&
         Height          =   285
         Left            =   3120
         TabIndex        =   13
         Top             =   240
         Width           =   855
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Disk:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   3
         Left            =   4080
         TabIndex        =   12
         Top             =   600
         Width           =   2175
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "File(s):"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   4
         Left            =   4080
         TabIndex        =   11
         Top             =   3000
         Width           =   1095
      End
   End
   Begin VB.CommandButton Options 
      Caption         =   "Options"
      Height          =   495
      Left            =   4920
      TabIndex        =   24
      Top             =   5760
      Width           =   735
   End
   Begin VB.CommandButton About 
      Caption         =   "About"
      Height          =   495
      Left            =   4920
      TabIndex        =   23
      Top             =   6360
      Width           =   735
   End
   Begin VB.Frame Frame2 
      Caption         =   "Local Drive"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   6735
      Left            =   120
      TabIndex        =   3
      Top             =   120
      Width           =   4695
      Begin VB.TextBox BlockText 
         Enabled         =   0   'False
         Height          =   285
         Left            =   3600
         TabIndex        =   32
         Text            =   "0 Blocks"
         Top             =   5760
         Width           =   975
      End
      Begin VB.TextBox KBText 
         Enabled         =   0   'False
         Height          =   285
         Left            =   3600
         TabIndex        =   31
         Text            =   "0 KB"
         Top             =   5400
         Width           =   975
      End
      Begin VB.CommandButton MakeDir 
         Caption         =   "Make Dir"
         Height          =   375
         Left            =   3600
         TabIndex        =   29
         Top             =   1320
         Width           =   975
      End
      Begin VB.DirListBox Dir1 
         Height          =   1665
         Left            =   120
         TabIndex        =   27
         Top             =   600
         Width           =   3375
      End
      Begin VB.FileListBox PCDirectory 
         Height          =   3795
         Left            =   120
         MultiSelect     =   2  'Extended
         TabIndex        =   20
         Top             =   2280
         Width           =   3375
      End
      Begin VB.CommandButton PCRefresh 
         Caption         =   "Refresh"
         Height          =   375
         Left            =   3600
         TabIndex        =   19
         Top             =   840
         Width           =   975
      End
      Begin VB.CommandButton RunFile 
         Caption         =   "Run / View"
         Height          =   375
         Left            =   3600
         TabIndex        =   18
         Top             =   3600
         Width           =   975
      End
      Begin VB.CommandButton PCRename 
         Caption         =   "Rename"
         Height          =   375
         Left            =   3600
         TabIndex        =   17
         Top             =   2640
         Width           =   975
      End
      Begin VB.TextBox PCWorkingDir 
         Height          =   285
         Left            =   120
         TabIndex        =   16
         Text            =   "c:\"
         Top             =   6360
         Width           =   4455
      End
      Begin VB.CommandButton PCDelete 
         Caption         =   "Delete"
         Height          =   375
         Left            =   3600
         TabIndex        =   15
         Top             =   3120
         Width           =   975
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Current Directory:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   7
         Left            =   120
         TabIndex        =   35
         Top             =   6120
         Width           =   2175
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Selected:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   6
         Left            =   3600
         TabIndex        =   30
         Top             =   5160
         Width           =   1095
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Disk:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   1
         Left            =   3600
         TabIndex        =   22
         Top             =   600
         Width           =   2175
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "File(s):"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Index           =   2
         Left            =   3600
         TabIndex        =   21
         Top             =   2400
         Width           =   1095
      End
   End
   Begin VB.CommandButton CopyFromFloppy 
      Caption         =   "<--"
      Height          =   495
      Left            =   4920
      TabIndex        =   2
      ToolTipText     =   "If no files are selected, this button will create a D64 image of the entire disk."
      Top             =   3240
      Width           =   735
   End
   Begin VB.CommandButton CopyToFloppy 
      Caption         =   "-->"
      Height          =   495
      Left            =   4920
      TabIndex        =   0
      Top             =   2640
      Width           =   735
   End
End
Attribute VB_Name = "MainForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

'-----------------
'GUI4CBM4WIN
'
' Copyright (C) 2004-2005 Leif Bloomquist
' Copyright (C) 2006      Wolfgang Moser
' Copyright (C) 2006      Spiro Trikaliotis
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
'   Additions by Wolfgang Moser, based on Gui4Cbm4Win 0.0.9
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


Private Sub About_Click()
    Dim CR As String
    CR = Chr$(13) & Chr$(13)
    MsgBox "GUI4CBM4WIN by Leif Bloomquist (no support anymore)" & Chr$(13) & _
           "adoption to OpenCBM 0.4.0 by Wolfgang Moser (http://d81.de/)            " & CR & _
           "This is a simple GUI front-end for cbm4win" & Chr$(13) & _
           "by Spiro Trikaliotis (www.trikaliotis.net/cbm4win/)" & CR & _
           "cbm4win itself is heavily based on cbm4linux," & Chr$(13) & _
           "written by Michael Klein (http://www.lb.shuttle.de/puffin/)" & CR & _
           "This is version 0.4.1 of the GUI," & Chr$(13) & _
           "distributed under the zlib/libpng OpenSource license." _
           , vbInformation, "About"
End Sub

'Called when user selects a file on the CBM directory
Private Sub CBMDirectory_Click()
    'Prevent "blocks free" from being selected
    If CBMDirectory.Selected(CBMDirectory.ListCount - 1) Then
        CBMDirectory.Selected(CBMDirectory.ListCount - 1) = False
    End If
End Sub

'Fetch the drive status strings.
Private Sub CBMDriveStatus_Click()
    Dim Status As ReturnStringType
    Status = DoCommand("cbmctrl", "status " & DriveNumber, "Reading drive status, please wait.")
    LastStatus.Caption = UCase(Status.Output)
End Sub

'Format a floppy.
Private Sub CBMFormat_Click()
    Dim Result
    Dim Status As ReturnStringType
    
    Result = MsgBox("This will erase ALL data on the floppy disk.  Are you sure?", vbExclamation Or vbYesNo, "Format Disk")
    If (Result = vbNo) Then Exit Sub

    Prompt.Ask "Please Enter Diskname, ID"
    If (Prompt.LastResult = CANCELSTRING) Then Exit Sub

    Status = DoCommand("cbmforng", " -vso " & DriveNumber & " " & Chr$(34) & UCase(Prompt.LastResult) & Chr$(34), "Formatting floppy disk, please wait.")
    LastStatus.Caption = UCase(Status.Output)
    Sleep 1000 'Just so message is visible
    
    RefreshCBMDir
End Sub

Private Sub CBMInitialize_Click()
    DoCommand "cbmctrl", "command " & DriveNumber & " I0", "Initializing Drive"
    CBMDriveStatus_Click
End Sub

Private Sub CBMRefresh_Click()

On Error GoTo CBMRefreshError:

    Dim CmdLine As String
    Dim temp As String
    Dim Temp2 As String
    Dim Results As ReturnStringType

    'Probably the most complicated action - read a directory.
    CBMDirectory.Clear
    
    'Run the program
    Results = DoCommand("cbmctrl", "dir " & DriveNumber, "Reading directory, please wait.", False)
    
    ' 'The drive status is always returned.
    ' LastStatus.Caption = UCase(Results.Errors)
    '
    ' It's not that simple anymore, we now need to extract it from the last line of stdout

    'Read in the complete output file -------------
    Close #1
    Open (Environ$("temp") & TEMPFILE1) For Input As #1
    
    'Check for empty file
    If EOF(1) Then Exit Sub
    
    'First line is dir. name and ID
    Line Input #1, temp
    CBMDiskName.Caption = UCase(ExtractQuotes(temp))
    CBMDiskID.Caption = UCase(Right$(temp, 5))

    If (Not EOF(1)) Then Line Input #1, temp

    While (Not EOF(1))
        ' Whenever another line is available, store the current one into the panel
        CBMDirectory.AddItem UCase(temp)  'Not only does uppercase look better, but Scratch, Rename need uppercase
        ' and get a new one
        Line Input #1, temp
    Wend
    'The drive status is taken from the last line on stdout
    LastStatus.Caption = UCase(temp)
    Close #1
    
    'And delete both temp files, so we're not cluttering things up
    Kill (Environ$("temp") & TEMPFILE1)
    Kill (Environ$("temp") & TEMPFILE2)
    
    Exit Sub
    
CBMRefreshError:

    If Not (Err.Number = 53) Then MsgBox "Error: " & Err.Description & " (" & Err.Number & ")  " & Chr$(13) & Chr$(13) & "Debug string: [" & temp & "]"
    ClearCBMDir
    Exit Sub
End Sub

Private Sub CBMRename_Click()
   Dim T As Integer

    For T = 0 To CBMDirectory.ListCount - 1
        If (CBMDirectory.Selected(T)) Then
            Prompt.Reply.Text = ExtractQuotes(CBMDirectory.List(T))
            Prompt.Ask "Enter new name for '" & ExtractQuotes(CBMDirectory.List(T)) & "'", False
            
            If Not (Prompt.LastResult = CANCELSTRING) Then
                DoCommand "cbmctrl", "command " & DriveNumber & " " & Chr$(34) & "R0:" & UCase(Prompt.LastResult) & "=" & ExtractQuotes(CBMDirectory.List(T)) & Chr$(34), "Renaming"
            Else
                Exit Sub
            End If
        End If
    Next T
    
   RefreshCBMDir
End Sub

Private Sub CBMReset_Click()
     DoCommand "cbmctrl", "reset", "Resetting drives, please wait."
End Sub

Private Sub CBMScratch_Click()
    Dim T As Integer
    
    For T = 0 To CBMDirectory.ListCount - 1
        If (CBMDirectory.Selected(T)) Then
            DoCommand "cbmctrl", "command " & DriveNumber & " " & Chr$(34) & "S0:" & ExtractQuotes(CBMDirectory.List(T)) & Chr$(34), "Scratching " & ExtractQuotes(CBMDirectory.List(T))
        End If
    Next T

    RefreshCBMDir
End Sub

Private Sub CBMValidate_Click()
     DoCommand "cbmctrl", "command " & DriveNumber & " " & Chr$(34) & "V0:" & Chr$(34), "Validating drive, please wait."
End Sub

Private Sub CopyFromFloppy_Click()
    Dim T As Integer
    Dim FilesSelected As Integer
    Dim Result
    Dim FileName As String
    Dim FileNameOut As String
    Dim FileNameTemp As String

    FilesSelected = 0
            
    ChDir PCWorkingDir.Text
    
    For T = 0 To CBMDirectory.ListCount - 1
        If (CBMDirectory.Selected(T)) Then
            FileName = LCase(ExtractQuotes(CBMDirectory.List(T)))
            
            'Check for SEQ file
            FileNameTemp = Replace(CBMDirectory.List(T), " ", "")  'Remove spaces first
            If (UCase(Right$(FileNameTemp, 3)) = "SEQ") Then
                FileNameOut = FileName & ".seq"
                FileName = FileName & ",s"
            Else
                FileNameOut = FileName
                'FileName stays the same
            End If
            
            DoCommand "cbmcopy", _
                      "--transfer=" & TransferString & " -q -r " & DriveNumber & " " & Chr$(34) & FileName & Chr$(34) & _
                      " --output=" & Chr$(34) & FileNameOut & Chr$(34), _
                      "Copying '" & ExtractQuotes(CBMDirectory.List(T)) & "' from floppy disk."
            FilesSelected = FilesSelected + 1
        End If
    Next T
    
    'No Files were selected, make a D64 instead.
    If (FilesSelected = 0) Then
        Result = MsgBox("No files selected.  Do you want to make a D64 image of this floppy disk?", vbQuestion Or vbYesNo, "Create D64")
        If (Result = vbNo) Then Exit Sub
        
        Prompt.Ask "Please Enter Filename:"
        If (Prompt.LastResult = CANCELSTRING) Then Exit Sub
    
        DoCommand "d64copy", "--transfer=" & TransferString & " " & NoWarpString & " " & DriveNumber & " " & Chr$(34) & Prompt.LastResult & Chr$(34), "Creating D64 image, please wait."
    End If

    PCDirectory.Refresh
End Sub

Private Sub CopyToFloppy_Click()
    Dim T As Integer
    Dim FilesSelected As Integer
    Dim FileName As String
    Dim FileNameOut As String
    Dim SeqType As String
    
    FilesSelected = 0
      
    For T = 0 To PCDirectory.ListCount - 1
        If (PCDirectory.Selected(T)) Then
            FilesSelected = FilesSelected + 1
        
            If (UCase(Right$(PCDirectory.List(T), 4)) = ".D64") Then 'Make Disk from D64
                WriteD64toFloppy PCDirectory.List(T)
                Exit Sub  'Exit so only 1 D64 is copied!
            Else 'Copy a File
                FileName = LCase(PCDirectory.List(T))
                
                'Remove .prg extention
                If (UCase(Right$(FileName, 4)) = ".PRG") Then
                    FileNameOut = Left$(FileName, Len(FileName) - 4)
                Else
                    FileNameOut = FileName
                End If
                
                'Change file type for .seq extension
                If (UCase(Right$(FileName, 4)) = ".SEQ") Then
                    SeqType = " --file-type S"
                    FileNameOut = Left$(FileName, Len(FileName) - 4)
                Else
                    SeqType = ""
                End If
                
                DoCommand "cbmcopy", _
                          "--transfer=" & TransferString & " -q -w " & DriveNumber & " " & Chr$(34) & FileName & Chr$(34) & _
                          " --output=" & Chr$(34) & FileNameOut & Chr$(34) & SeqType, _
                          "Copying '" & PCDirectory.List(T) & "' to floppy disk as '" & UCase(FileNameOut) & "'"
            End If
        End If
    Next T

    If (FilesSelected > 0) Then
        RefreshCBMDir
    End If
End Sub

Private Sub Dir1_Change()
    PCWorkingDir = Dir1.List(Dir1.ListIndex) & "\"
    ChDir PCWorkingDir
    PCRefresh_Click
End Sub

Private Sub Drive1_Change()

On Error Resume Next

    ChDrive Drive1.Drive
    ChDir CurDir
    Dir1.path = CurDir
    Dir1_Change
End Sub

Private Sub WriteD64toFloppy(d64file As String)
    Dim Result
    
    Result = MsgBox("This will overwrite ALL data on the floppy disk.  Are you sure?", vbExclamation Or vbYesNo, "Write D64 to Disk")
    If (Result = vbNo) Then Exit Sub

    DoCommand "d64copy", "--transfer=" & TransferString & " " & NoWarpString & " " & Chr$(34) & d64file & Chr$(34) & " " & DriveNumber, "Creating disk from D64 image, please wait."
    
    CBMRefresh_Click
End Sub

' Program Initialization
Private Sub Form_Load()

On Error Resume Next

    If (Command = "-leifdevelopment") Then
        ChDrive "E"
        ChDir "\_DDK_Test\cbm4win\0.4.0"
        OptionsForm.PreviewCheck.value = vbChecked
    End If
    
    CheckExes
    ExeDir = AddSlash(CurDir)

    LoadINI
    GotoDir OptionsForm.StartingPath.Text
    DriveNumber = OptionsForm.DriveNum.Text
    
    PCWorkingDir.Text = AddSlash(CurDir)
    Dir1.path = CurDir
    PCRefresh_Click
End Sub

'A slightly smarter version of Chdir, that handles drives as well.
Private Sub GotoDir(FullPath As String)

On Error GoTo GotoDirError:

    'Extract the drive letter
    Dim Drive As String
    Dim PathOnly As String
    
    Drive = Left$(FullPath, 1)
    ChDrive Drive
    Drive1.Drive = Drive
    
    PathOnly = Right$(FullPath, Len(FullPath) - 2) 'Strip off Drive Letter and :
    ChDir PathOnly
    
    Exit Sub
    
GotoDirError:
     MsgBox "Error: " & Err.Description & " (" & Err.Number & ") in GotoDir() - " & FullPath
End Sub


'This function must be private, because of the return type.
Private Function DoCommand(Action As String, Args As String, WaitMessage As String, Optional DeleteOutFile As Boolean = True) As ReturnStringType

On Error Resume Next
    
    Static InProgress As Boolean
    Dim CmdLine As String
    Dim ErrorString As String
    
    If (InProgress) Then
        MsgBox "cbm4win command already in progress, cannot continue.", vbCritical
        Exit Function
    End If
    
    Close #1
    Close #2
    
    'Check command - mostly for debugging.
    Dim Result
    If (OptionsForm.PreviewCheck.value) Then
        Result = MsgBox("Requested command:" & Chr$(13) & Chr$(13) & Action & " " & Args & Chr$(13) & Chr$(13) & "OK to continue?", vbYesNo)
        If Result = vbNo Then Exit Function
    End If
    
    'Flag that the background process is starting.
    InProgress = True
    
    'Show in Progress Dialog
    ' Waiting.Show
    Waiting.Show vbModeless, MainForm
    Waiting.Label = WaitMessage
    
    'cmd /c is needed in order to have a shell write to a file (long, complicated explanation)
    '1> redirects stdout to a file, and 2> redirects stderr to a file (Win2K/XP only)
    
    'All these quotes [chr$(34)] are needed to handle spaces.  So you get:
    'cmd /c ""path\command" args "files""
    
    CmdLine = "cmd /c " & Chr$(34) & Chr$(34) & ExeDir & Action & Chr$(34) & " " & Args & Chr$(34) & _
              " 1>" & Environ$("temp") & TEMPFILE1 & " 2>" & Environ$("temp") & TEMPFILE2
    ShellWait CmdLine, vbHide
    
    'Read in the output file
    Open (Environ$("temp") & TEMPFILE1) For Input As #1
        If (Not EOF(1)) Then Line Input #1, DoCommand.Output
    Close #1
    
    'Read in the error file
    Open (Environ$("temp") & TEMPFILE2) For Input As #2
        ' If (Not EOF(2)) Then Line Input #2, DoCommand.Errors
        If (Not EOF(2)) Then Line Input #2, ErrorString
        ' The following is useful for reading multiline messages
        ' If (Not EOF(2)) Then ErrorString = Input(LOF(2), #2)
    Close #2

    'And delete both, so we're not cluttering things up
    If (DeleteOutFile) Then
        Kill (Environ$("temp") & TEMPFILE1)
        Kill (Environ$("temp") & TEMPFILE2)
    End If
    
    'Old, unsafe way
    'Shell ExeDir & Action, vbHide
    
    Waiting.Hide
    InProgress = False

    If (Not ErrorString = "") Then MsgBox ErrorString, vbOKOnly, Action
End Function

Private Sub MakeDir_Click()
    Prompt.Ask "Enter Directory Name"
    If (Prompt.LastResult = CANCELSTRING) Then Exit Sub
    
    MkDir AddSlash(Dir1.path) & Prompt.LastResult
    PCRefresh_Click
End Sub

Private Sub Options_Click()
    OptionsForm.StartingPath.Text = AddSlash(PCWorkingDir.Text)

    OptionsForm.Show vbModal

    DriveNumber = OptionsForm.DriveNum.Text
    PCWorkingDir.Text = OptionsForm.StartingPath.Text
    GotoDir PCWorkingDir.Text
End Sub

Private Sub PCDelete_Click()
    Dim T As Integer
    Dim Result
    
    Result = MsgBox("Delete the selected file(s)?", vbYesNo Or vbQuestion, "Delete Files")
    If (Result = vbNo) Then Exit Sub
    
    For T = 0 To PCDirectory.ListCount - 1
        If (PCDirectory.Selected(T)) Then
            Kill PCDirectory.List(T)
        End If
    Next T
    
    PCDirectory.Refresh
End Sub

Private Sub PCDirectory_Click()
    Dim T As Integer
    Dim Bytes As Long
    Dim D64Selected As Integer
    
    D64Selected = -1
    Bytes = 0
    
    'If a D64 is selected, deselect all the others
    For T = 0 To PCDirectory.ListCount - 1
        If (PCDirectory.Selected(T) And UCase(Right$(PCDirectory.List(T), 4)) = ".D64") Then
            D64Selected = T
            Exit For
        End If
    Next T
    
    If (D64Selected >= 0) Then
        For T = 0 To PCDirectory.ListCount - 1
            If Not (T = D64Selected) Then
                PCDirectory.Selected(T) = False
            End If
        Next T
    End If
    
    'Refresh the KB/Blocks display
    For T = 0 To PCDirectory.ListCount - 1
        If (PCDirectory.Selected(T)) Then
            ' Check if file exists first
            If (Dir(AddSlash(Dir1.path) & PCDirectory.List(T)) = "") Then
                ' File does not exist anymore
                PCDirectory.Refresh
                Exit For
            Else
                Bytes = Bytes + FileLen(AddSlash(Dir1.path) & PCDirectory.List(T))
            End If
        End If
    Next T
    
    KBText.Text = Format(Bytes / 1024, "0.0") & " KB"
    BlockText.Text = CInt(Bytes / 254) & " Blocks"      '254 Bytes per C= Block
End Sub

Private Sub PCRefresh_Click()
    PCWorkingDir.Text = AddSlash(CurDir)
    PCDirectory.path = CurDir
    PCDirectory.Refresh
    Dir1.Refresh
End Sub

Private Sub PCRename_Click()
    Dim T As Integer

    For T = 0 To PCDirectory.ListCount - 1
        If (PCDirectory.Selected(T)) Then
            Prompt.Reply.Text = PCDirectory.List(T)
            Prompt.Ask "Enter new name for '" & PCDirectory.List(T) & "'", False
            
            If Not (Prompt.LastResult = CANCELSTRING) Then
                Name PCDirectory.List(T) As Prompt.LastResult
            End If
        End If
    Next T
    
    PCDirectory.Refresh
End Sub

Private Sub PCWorkingDir_KeyDown(KeyCode As Integer, Shift As Integer)

On Error Resume Next

    'Enable Enter Key
    If (KeyCode = 13) Then
        ' ChDir PCWorkingDir.Text
        GotoDir PCWorkingDir.Text
        PCRefresh_Click
        Dir1.path = PCWorkingDir.Text
    End If
End Sub

'Thanks to vinnyd79 on Experts Exchange for this function!
Private Function ShellWait(PathName, Optional WindowStyle As VbAppWinStyle = vbNormalFocus) As Double

Dim hProcess As Long, RetVal As Long

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, False, Shell(PathName, WindowStyle))
    Do
        GetExitCodeProcess hProcess, RetVal
        DoEvents: Sleep 10 ' was 100
    Loop While RetVal = STILL_ACTIVE
End Function

Private Function ExtractQuotes(FullString As String) As String

On Error GoTo QuoteError:

    Dim Quote1 As Integer
    Dim Quote2 As Integer
    
    Quote1 = InStr(FullString, Chr$(34))
    Quote2 = InStr(Quote1 + 1, FullString, Chr$(34))
    ExtractQuotes = Mid$(FullString, Quote1 + 1, Quote2 - Quote1 - 1)
    
    Exit Function
    
QuoteError:
     MsgBox "Error: " & Err.Description & " (" & Err.Number & ")  " & Chr$(13) & Chr$(13) & "Debug string: [" & FullString & "]"
    
End Function

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    'Make sure all child forms are closed
    End
End Sub

Private Sub RunFile_Click()
  Dim T As Integer
  Dim hWnd
 
    For T = 0 To PCDirectory.ListCount - 1
        If (PCDirectory.Selected(T)) Then
            ShellExecute hWnd, "open", PCDirectory.List(T), vbNullString, CurDir, 1
            
            'Stop so only the first file is executed
            Exit Sub
        End If
    Next T
End Sub

Public Function PubDoCommand(Action As String, Args As String, WaitMessage As String, Optional DeleteOutFile As Boolean = True) As String
    Dim Returns As ReturnStringType
    Returns = DoCommand(Action, Args, WaitMessage, DeleteOutFile)
    PubDoCommand = Returns.Output
End Function

'Check that the program is in the correct place, berate user if not :-)
Private Sub CheckExes()
    Dim FilesOK As Boolean
    FilesOK = Not (Dir("cbmctrl.exe") = "")

    If (FilesOK) Then Exit Sub
    
    MsgBox "Can't find cbmctrl.exe " & Chr$(13) & Chr$(13) & "gui4cbm4win must be run from within the same directory as the cbm4win executable files." & _
           Chr$(13) & Chr$(13) & "Current Directory: " & CurDir, _
           vbCritical, "Error"
    End
End Sub

'Refresh the directory - only if automatic refresh hasn't been turned off
Private Sub RefreshCBMDir()
    If (OptionsForm.AutoRefreshDir.value) Then
        CBMRefresh_Click
    Else
        ClearCBMDir
    End If
End Sub

'Clear the directory listing, because contents have changed
Private Sub ClearCBMDir()
    CBMDirectory.Clear
    CBMDiskName.Caption = ""
    CBMDiskID.Caption = ""
    LastStatus.Caption = ""
End Sub

