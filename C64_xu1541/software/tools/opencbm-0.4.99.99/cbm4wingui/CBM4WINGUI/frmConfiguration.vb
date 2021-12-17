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
#End Region

Imports System.Configuration

Public Class frmConfiguration

    Const INITIAL_DIRECTORY As String = "InitialDirectory"
    Const DEFAULT_CBM_DRIVE As String = "DefaultCbmDrive"
    Const DISABLE_WARP As String = "DisableWarp"
    Const PREVIEW_COMMANDS As String = "PreviewCommands"
    Const REFRESH_AFTER As String = "RefreshAfter"
    Const TRANSFER_STRING As String = "TransferString"

    Dim WithEvents m_objCbmDevicesTable As CbmDevices.CbmDevicesDataTable

    Dim m_intDefaultDevceNumber As Integer = 0
    Dim m_blnDisableWarp As Boolean = False

    Public Property DisableWarp() As Boolean
        Get
            Return m_blnDisableWarp
        End Get
        Set(ByVal value As Boolean)
            m_blnDisableWarp = value
            m_chkDisableWarp.Checked = value

            If value Then
                g_strNoWarp = "--no-warp"
            Else
                g_strNoWarp = ""
            End If
        End Set
    End Property

    Public Property InitialDirectory() As String
        Get
            Return m_txtInitialDirectory.Text
        End Get
        Set(ByVal value As String)
            m_txtInitialDirectory.Text = value

            If m_txtInitialDirectory.Text.Length = 0 Then
                m_txtInitialDirectory.Text = System.IO.Path.Combine( _
                    Environment.GetFolderPath( _
                        Environment.SpecialFolder.ApplicationData), _
                    "GUI4CBM4WIN")

                If Not System.IO.Directory.Exists(m_txtInitialDirectory.Text) Then
                    System.IO.Directory.CreateDirectory(m_txtInitialDirectory.Text)
                End If
            End If
        End Set
    End Property

    Private Sub m_btnCancel_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_btnCancel.Click
        LoadConfig()
        Me.DialogResult = Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub

    Private Sub m_btnDone_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_btnDone.Click
        SaveConfig()
        Me.DialogResult = Windows.Forms.DialogResult.OK
        Me.Close()
    End Sub

    Public Sub LoadConfig()

        With Config.AppSettings

            If Not (.Settings(INITIAL_DIRECTORY) Is Nothing) Then
                InitialDirectory = .Settings(INITIAL_DIRECTORY).Value
            End If

            If Not (.Settings(DEFAULT_CBM_DRIVE) Is Nothing) Then
                DefaultDeviceNumber = CInt(.Settings(DEFAULT_CBM_DRIVE).Value)
            End If

            If Not (.Settings(DISABLE_WARP) Is Nothing) Then
                DisableWarp = CBool(.Settings(DISABLE_WARP).Value)
            Else
                DisableWarp = False
            End If

            If Not (.Settings(PREVIEW_COMMANDS) Is Nothing) Then
                PreviewCommands = CBool(.Settings(PREVIEW_COMMANDS).Value)
            Else
                PreviewCommands = False
            End If

            If Not (.Settings(REFRESH_AFTER) Is Nothing) Then
                RefreshAfter = CBool(.Settings(REFRESH_AFTER).Value)
            Else
                RefreshAfter = True
            End If

            If Not (.Settings(TRANSFER_STRING) Is Nothing) Then
                TransferString = .Settings(TRANSFER_STRING).Value

                Select Case TransferString
                    Case "auto"
                        m_radAutomatic.Checked = True

                    Case "original"
                        m_radOriginal.Checked = True

                    Case "serial1"
                        m_radSerial1.Checked = True

                    Case "serial2"
                        m_radSerial2.Checked = True

                    Case "prallel"
                        m_radParallel.Checked = True

                End Select
            Else
                TransferString = "auto"
                m_radAutomatic.Checked = True
            End If

        End With

    End Sub

    Private Sub SaveConfig()

        ' PC Options
        SetProperty(INITIAL_DIRECTORY, m_txtInitialDirectory.Text.Trim())


        ' OpenCBM Options
        SetProperty(DEFAULT_CBM_DRIVE, m_intDefaultDevceNumber.ToString())
        SetProperty(DISABLE_WARP, m_blnDisableWarp.ToString())
        SetProperty(PREVIEW_COMMANDS, m_chkPreviewCommands.Checked.ToString())
        SetProperty(REFRESH_AFTER, m_chkRefreshDirectory.Checked.ToString())
        SetProperty(TRANSFER_STRING, TransferString)

        ' Emulator Options


        Config.Save()

    End Sub

    Private Sub SetProperty(ByVal Name As String, ByVal Value As String)
        With Config.AppSettings.Settings
            If .Item(Name) Is Nothing Then
                .Add(Name, Value)
            Else
                .Item(Name).Value = Value
            End If
        End With
    End Sub

    Private Sub m_objTools_ItemSelected( _
        ByVal sender As System.Object, _
        ByVal e As Pabo.MozBar.MozItemEventArgs) _
            Handles m_objTools.ItemSelected

        Dim strSelected As String = e.MozItem.Tag

        Select Case strSelected
            Case "PC"
                m_objTabs.SelectedIndex = 0

            Case "OpenCBM"
                m_objTabs.SelectedIndex = 1

            Case "Emulator"
                m_objTabs.SelectedIndex = 2

        End Select

    End Sub

    Private Sub m_btnHelp_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_btnHelp.Click
        MessageBox.Show("Coming Soon", "Not Yet Implemented", MessageBoxButtons.OK, MessageBoxIcon.Information)
    End Sub

    Private Sub m_btnBrowse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_btnBrowse.Click

        m_objFolder.SelectedPath = ConfigurationManager.AppSettings(INITIAL_DIRECTORY)

        If m_objFolder.ShowDialog(Me) = Windows.Forms.DialogResult.OK Then
            m_txtInitialDirectory.Text = m_objFolder.SelectedPath
        End If

    End Sub

    Private Sub frmConfiguration_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        LoadConfig()

        m_objCbmDevicesTable = m_objCbmDevices._CbmDevices
    End Sub

    Private Sub m_btnResetBus_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_btnResetBus.Click
        frmMain.PubDoCommand("cbmctrl", "reset", "Resetting drives, please wait.")
    End Sub

    Private Sub m_chkDisableWarp_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_chkDisableWarp.CheckedChanged
        DisableWarp = m_chkDisableWarp.Checked
    End Sub

    Public Property DefaultDeviceNumber() As Integer
        Get
            If m_intDefaultDevceNumber = 0 Then
                Try
                    m_intDefaultDevceNumber = CInt(Config.AppSettings.Settings(DEFAULT_CBM_DRIVE).Value)
                Catch ex As Exception
                    m_intDefaultDevceNumber = 8
                End Try
            End If
            Return m_intDefaultDevceNumber
        End Get
        Set(ByVal value As Integer)
            m_intDefaultDevceNumber = value
            frmMain.SetCbmDrive()
        End Set
    End Property

    Private Sub m_btnDetectDevices_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles m_btnDetectDevices.Click
        Detect()
    End Sub

    Dim m_blnDetecting As Boolean = False

    Private Sub Detect()
        m_blnDetecting = True
        Dim strResult As String = _
            frmMain.PubDoCommand("cbmctrl", "detect", "Detecting Drives...", False)


        Dim objStringReader As New System.IO.StringReader(strResult)

        Dim strLine As String = objStringReader.ReadLine()

        m_objCbmDevices.Clear()
        m_objCbmDevices.AcceptChanges()

        m_objCbmDevices.BeginInit()
        Do
            If strLine.IndexOf(":") > -1 Then
                Dim objNewRow As CbmDevices.CbmDevicesRow = _
                    m_objCbmDevices._CbmDevices.NewCbmDevicesRow()

                Dim strDriveNumber As String = strLine.Split(CChar(":"))(0).Trim()
                Dim strModel As String = strLine.Split(CChar(":"))(1).Trim()

                objNewRow.Device = CInt(strDriveNumber)

                If (objNewRow.Device = DefaultDeviceNumber) Then
                    objNewRow._Default = True
                Else
                    objNewRow._Default = False
                End If

                objNewRow.Model = strModel

                objNewRow.Table.Rows.Add(objNewRow)

                strLine = objStringReader.ReadLine()
            End If
        Loop Until strLine Is Nothing Or strLine = vbNullString
        m_objCbmDevices.EndInit()

        m_objCbmDevices.AcceptChanges()
        m_blnDetecting = False
    End Sub

    Public Property PreviewCommands() As Boolean
        Get
            Return m_chkPreviewCommands.Checked
        End Get
        Set(ByVal value As Boolean)
            m_chkPreviewCommands.Checked = value
        End Set
    End Property

    Public Property RefreshAfter() As Boolean
        Get
            Return m_chkRefreshDirectory.Checked
        End Get
        Set(ByVal value As Boolean)
            m_chkRefreshDirectory.Checked = value
        End Set
    End Property

    Private Sub m_objCbmDevicesTable_ColumnChanged( _
        ByVal sender As Object, _
        ByVal e As System.Data.DataColumnChangeEventArgs) _
            Handles m_objCbmDevicesTable.ColumnChanged

        If Not m_blnDetecting And _
            e.Column.Caption = "Is Default" And _
            CBool(e.ProposedValue) Then

            DefaultDeviceNumber = e.Row("Device")

            For Each objRow As CbmDevices.CbmDevicesRow In m_objCbmDevicesTable.Rows
                If objRow.Device <> DefaultDeviceNumber Then
                    objRow._Default = False
                End If
            Next
        End If

    End Sub

    Private Sub m_radAutomatic_CheckedChanged( _
        ByVal sender As System.Object, _
        ByVal e As System.EventArgs) _
            Handles m_radParallel.CheckedChanged, _
                m_radSerial2.CheckedChanged, _
                m_radSerial1.CheckedChanged, _
                m_radOriginal.CheckedChanged, _
                m_radAutomatic.CheckedChanged

        Dim blnPerform As Boolean = True

        If CType(sender, RadioButton).Name = "m_radParallel" And m_radParallel.Checked Then
            blnPerform = False

            If MessageBox.Show( _
                String.Format("Do you have a parallel cable installed on drive {0}?", DefaultDeviceNumber), _
                "Confirm", MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2) = Windows.Forms.DialogResult.Yes Then

                blnPerform = True
            End If
        End If

        If (blnPerform) Then
            TransferString = CType(sender, RadioButton).Tag
        Else
            m_radAutomatic.Checked = True
        End If

    End Sub
End Class