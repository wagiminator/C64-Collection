#Region "Copyright"
'-----------------
'GUI4CBM4WIN
'
' Copyright (C) 2004-2005 Leif Bloomquist
' Copyright (C) 2006      Wolfgang Moser
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
#End Region

Option Strict Off
Option Explicit On
Imports System.IO

Friend Class OptionsForm
    Inherits System.Windows.Forms.Form



    Private IsOnTop As Boolean

    Private Sub ApplyChanges_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles ApplyChanges.Click
        SaveINI()
        Me.Hide()
    End Sub

    Private Sub Morse_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs)
        ChDir(ExeDir)
        frmMain.PubDoCommand("morse", "" & DriveNum.Text, "Morse Code Demo")
    End Sub

    Public WriteOnly Property AlwaysOnTop() As Boolean
        Set(ByVal Value As Boolean)
            Dim lFlag As Integer
            If Value Then lFlag = HWND_TOPMOST Else lFlag = HWND_NOTOPMOST
            IsOnTop = Value
            SetWindowPos(Me.Handle.ToInt32, lFlag, 0, 0, 0, 0, (SWP_NOSIZE Or SWP_NOMOVE))
        End Set
    End Property

    Private Sub Cancel_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles Cancel.Click
        Me.Hide()
    End Sub

    Private Sub OptionsForm_Load(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles MyBase.Load
        Me.AlwaysOnTop = True
    End Sub

    Private Sub CBMDetect_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs)

        'Read in the complete output file -------------
        If (Result = "") Then Result = "No drives found, please check cbm4win installation and directory paths!"
        MsgBox(Result, MsgBoxStyle.OkOnly, "Drive Detection")
    End Sub

    Private Sub ResetBus_Click(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs)
        frmMain.PubDoCommand("cbmctrl", "reset", "Resetting drives, please wait.")
    End Sub

    'UPGRADE_WARNING: Event Transfer.CheckedChanged may fire when form is initialized. Click for more: 'ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?keyword="88B12AE1-6DE0-48A0-86F1-60C0686C026A"'
    Private Sub Transfer_CheckedChanged(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles Transfer.CheckedChanged
        If eventSender.Checked Then
            Dim Index As Short = Transfer.GetIndex(eventSender)
            Select Case Index
                Case 0
                    TransferString = "original"
                Case 1
                    TransferString = "serial1"
                Case 2
                    TransferString = "serial2"
                Case 3
                    TransferString = "parallel"
                Case 4
                    TransferString = "auto"
            End Select
        End If
    End Sub

    Private Sub cmdBrowse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        If _BrowseFolders.ShowDialog(Me) = Windows.Forms.DialogResult.OK Then
            StartingPath.Text = _BrowseFolders.SelectedPath
        End If
    End Sub

    Private Sub CheckNoWarpMode_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

    End Sub
End Class