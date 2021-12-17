VERSION 5.00
Begin VB.Form Waiting 
   BorderStyle     =   1  'Fest Einfach
   Caption         =   "Working..."
   ClientHeight    =   1391
   ClientLeft      =   39
   ClientTop       =   325
   ClientWidth     =   2795
   ControlBox      =   0   'False
   LinkTopic       =   "Form2"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1391
   ScaleWidth      =   2795
   StartUpPosition =   1  'Fenstermitte
   Begin VB.Timer LEDTimer 
      Interval        =   300
      Left            =   600
      Top             =   840
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   2160
      TabIndex        =   1
      Top             =   720
      Visible         =   0   'False
      Width           =   1455
   End
   Begin VB.Shape LED 
      BackColor       =   &H00000000&
      BackStyle       =   1  'Undurchsichtig
      BorderWidth     =   2
      Height          =   255
      Left            =   1080
      Top             =   960
      Width           =   615
   End
   Begin VB.Label Label 
      Alignment       =   2  'Zentriert
      BackStyle       =   0  'Transparent
      Caption         =   "Please wait."
      Height          =   735
      Left            =   8
      TabIndex        =   0
      Top             =   240
      Width           =   2775
   End
End
Attribute VB_Name = "Waiting"
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

Public Property Let AlwaysOnTop(ByVal bState As Boolean)
  Dim lFlag As Long
  If bState Then lFlag = HWND_TOPMOST Else lFlag = HWND_NOTOPMOST
  IsOnTop = bState
  SetWindowPos Me.hWnd, lFlag, 0&, 0&, 0&, 0&, (SWP_NOSIZE Or SWP_NOMOVE)
End Property

Private Sub Form_Load()
    Me.AlwaysOnTop = True
End Sub

'There's no "elegant" way to abort a running cbm4win process, short of killing the PID, so this is left for future...

Private Sub Cancel_Click()
    Me.Hide
End Sub

Private Sub LEDTimer_Timer()
    If LED.BackColor = vbRed Then
        LED.BackColor = vbBlack
    Else
        LED.BackColor = vbRed
    End If
End Sub
