VERSION 5.00
Begin VB.Form OptionsForm 
   BorderStyle     =   1  'Fest Einfach
   Caption         =   "Options"
   ClientHeight    =   5603
   ClientLeft      =   39
   ClientTop       =   325
   ClientWidth     =   6292
   Icon            =   "Options.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5603
   ScaleWidth      =   6292
   StartUpPosition =   3  'Windows-Standard
   Begin VB.CommandButton ApplyChanges 
      Caption         =   "Save+Apply Changes"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   7.62
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   360
      TabIndex        =   10
      Top             =   4914
      Width           =   3135
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   7.62
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4095
      TabIndex        =   9
      Top             =   4914
      Width           =   1695
   End
   Begin VB.Frame Frame 
      Caption         =   "Preferences"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   7.62
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H00000000&
      Height          =   4628
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   6015
      Begin VB.OptionButton Transfer 
         Caption         =   "Auto (Recommended)"
         Height          =   255
         Index           =   4
         Left            =   117
         TabIndex        =   18
         ToolTipText     =   "Let OpenCBM select the most efficient transfer mode"
         Top             =   3400
         Value           =   -1  'True
         Width           =   3015
      End
      Begin VB.CheckBox AutoRefreshDir 
         Caption         =   "Automatically refresh directory after write to floppy"
         Height          =   375
         Left            =   120
         TabIndex        =   17
         Top             =   4095
         Value           =   1  'Aktiviert
         Width           =   5175
      End
      Begin VB.OptionButton Transfer 
         Caption         =   "Original (Very Slow!)"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   15
         Top             =   2400
         Width           =   3015
      End
      Begin VB.CheckBox PreviewCheck 
         Caption         =   "Preview CBM4WIN commands"
         Height          =   375
         Left            =   120
         TabIndex        =   14
         Top             =   3744
         Width           =   5175
      End
      Begin VB.Frame Frame 
         Caption         =   "System Tools"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   7.62
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000000&
         Height          =   1976
         Index           =   1
         Left            =   3960
         TabIndex        =   11
         Top             =   960
         Width           =   1815
         Begin VB.CommandButton ResetBus 
            Caption         =   "Reset Bus"
            Height          =   375
            Left            =   234
            TabIndex        =   19
            ToolTipText     =   "Reset all device on the IEC bus"
            Top             =   1320
            Width           =   1335
         End
         Begin VB.CommandButton CBMDetect 
            Caption         =   "Detect Drive"
            Height          =   375
            Left            =   240
            TabIndex        =   13
            ToolTipText     =   "Detect all currently active device on the IEC bus"
            Top             =   360
            Width           =   1335
         End
         Begin VB.CommandButton Morse 
            Caption         =   "Morse Code"
            Height          =   375
            Left            =   240
            TabIndex        =   12
            ToolTipText     =   "Send a SOS morse code to the LED of the currently selected device"
            Top             =   840
            Width           =   1335
         End
      End
      Begin VB.OptionButton Transfer 
         Caption         =   "Parallel"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   8
         ToolTipText     =   "Requires a XP1541/XP1571 cable"
         Top             =   3150
         Width           =   3015
      End
      Begin VB.OptionButton Transfer 
         Caption         =   "Serial 2"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   7
         ToolTipText     =   "This only works with one serial device connected."
         Top             =   2900
         Width           =   3015
      End
      Begin VB.OptionButton Transfer 
         Caption         =   "Serial 1 (Slow!)"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   6
         Top             =   2650
         Width           =   3015
      End
      Begin VB.CheckBox CheckNoWarpMode 
         Caption         =   "Disable Warp Mode for D64 Transfer"
         Height          =   375
         Left            =   120
         TabIndex        =   5
         Top             =   1680
         Width           =   2895
      End
      Begin VB.ComboBox DriveNum 
         Height          =   273
         ItemData        =   "Options.frx":0442
         Left            =   117
         List            =   "Options.frx":0452
         Style           =   2  'Dropdown-Liste
         TabIndex        =   3
         Top             =   1170
         Width           =   858
      End
      Begin VB.TextBox StartingPath 
         Height          =   285
         Left            =   120
         TabIndex        =   1
         Text            =   "c:\"
         Top             =   480
         Width           =   5415
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Transfer Mode:"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   16
         Top             =   2160
         Width           =   2415
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Drive Number:"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   4
         Top             =   960
         Width           =   2415
      End
      Begin VB.Label Label 
         BackStyle       =   0  'Transparent
         Caption         =   "Default location of your .d64, .prg, and .seq files:"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   2
         Top             =   240
         Width           =   5415
      End
   End
End
Attribute VB_Name = "OptionsForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (C) 2004-2005 Leif Bloomquist
' Copyright (C) 2006      Wolfgang Moser
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

Option Explicit

Private IsOnTop As Boolean

Private Sub ApplyChanges_Click()
    SaveINI
    Me.Hide
End Sub

Private Sub CheckNoWarpMode_Click()
    If CheckNoWarpMode.value = vbChecked Then
        NoWarpString = "--no-warp"
    Else
        NoWarpString = ""
    End If
End Sub

Private Sub Morse_Click()
    ChDir ExeDir
    MainForm.PubDoCommand "morse", "" & DriveNum.Text, "Morse Code Demo"
End Sub

Public Property Let AlwaysOnTop(ByVal bState As Boolean)
  Dim lFlag As Long
  If bState Then lFlag = HWND_TOPMOST Else lFlag = HWND_NOTOPMOST
  IsOnTop = bState
  SetWindowPos Me.hWnd, lFlag, 0&, 0&, 0&, 0&, (SWP_NOSIZE Or SWP_NOMOVE)
End Property

Private Sub Cancel_Click()
    Me.Hide
End Sub

Private Sub Form_Load()
    Me.AlwaysOnTop = True
End Sub

Private Sub CBMDetect_Click()
    Dim What As String
    MainForm.PubDoCommand "cbmctrl", "detect", "Detecting Drives...", False

    'Read in the complete output file -------------
    Close #1
    Open (Environ$("temp") & TEMPFILE1) For Input As #1
        What = Input(LOF(1), #1)
    Close #1

    'And delete both temp files, so we're not cluttering things up
    Kill (Environ$("temp") & TEMPFILE1)
    Kill (Environ$("temp") & TEMPFILE2)
    
    If (What = "") Then What = "No drives found, please check cbm4win installation and directory paths!"
    MsgBox What, vbOKOnly, "Drive Detection"
End Sub

Private Sub ResetBus_Click()
    MainForm.PubDoCommand "cbmctrl", "reset", "Resetting drives, please wait."
End Sub

Private Sub Transfer_Click(Index As Integer)
    Select Case Index
        Case 0:
            TransferString = "original"
        Case 1:
            TransferString = "serial1"
        Case 2:
            TransferString = "serial2"
        Case 3:
            TransferString = "parallel"
        Case 4:
            TransferString = "auto"
    End Select
End Sub
