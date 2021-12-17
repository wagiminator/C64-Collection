VERSION 5.00
Begin VB.Form Prompt 
   BorderStyle     =   1  'Fest Einfach
   ClientHeight    =   2171
   ClientLeft      =   39
   ClientTop       =   325
   ClientWidth     =   4017
   Icon            =   "Prompt.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2171
   ScaleWidth      =   4017
   StartUpPosition =   3  'Windows-Standard
   Begin VB.TextBox Reply 
      Height          =   285
      Left            =   360
      TabIndex        =   0
      Top             =   1080
      Width           =   3135
   End
   Begin VB.CommandButton OK 
      Caption         =   "OK"
      Height          =   375
      Left            =   360
      TabIndex        =   3
      Top             =   1560
      Width           =   1335
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   2160
      TabIndex        =   1
      Top             =   1560
      Width           =   1335
   End
   Begin VB.Label Label 
      Alignment       =   2  'Zentriert
      BackStyle       =   0  'Transparent
      Caption         =   "Question"
      Height          =   495
      Left            =   0
      TabIndex        =   2
      Top             =   120
      Width           =   3975
   End
End
Attribute VB_Name = "Prompt"
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

Public LastResult As String

Public Sub Ask(Q As String, Optional ClearLast = True)
    LastResult = ""
    Label.Caption = Q
    
    If (ClearLast) Then Reply.Text = ""
    
    Me.Show vbModal
End Sub


Private Sub Cancel_Click()
    LastResult = CANCELSTRING
    Me.Hide
End Sub

Private Sub OK_Click()
    LastResult = Reply.Text
    Me.Hide
End Sub

Private Sub Reply_KeyDown(KeyCode As Integer, Shift As Integer)
    'Enable Enter Key
    If (KeyCode = 13) Then OK_Click
End Sub
