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

Option Strict Off
Option Explicit On
Module INIRoutines

	
	
	Const INIFILE As String = "gui4cbm4win.ini"
	
	'Global Variables
	Public ExeDir As String
	Public DriveNumber As Short
    Public g_strNoWarp As String
    Public TransferString As String = "auto"
    Public AutoRefreshDir As Boolean

    'Load the INI file
    Public Sub LoadINI()

        On Error GoTo LoadINIError 'Get through as much as possible

        'Set Defaults
        'OptionsForm.StartingPath.Text = CurDir()
        'OptionsForm.DriveNum.Text = "8"

        ' Let the "Disable-Warp" option unselected
        ' NoWarpString = "--no-warp"
        ' OptionsForm.CheckNoWarpMode.value = vbChecked

        'OptionsForm.Transfer(4).Checked = True
        'TransferString = "auto"

        'AutoRefreshDir = True
        'OptionsForm.AutoRefreshDir.CheckState = System.Windows.Forms.CheckState.Checked

        'Load the parameters from the ini file, overriding the defaults

        'FileOpen(1, INIFILE, OpenMode.Input)
        'With OptionsForm
        ' .StartingPath.Text = AddSlash(GetINIValue("StartingPath"))

        '.DriveNum.Text = GetINIValue("DriveNumber")
        'DriveNumber = CShort(.DriveNum.Text)

        'g_strNoWarp = GetINIValue("NoWarpString")
        '.CheckNoWarpMode.CheckState = Val(CStr(-1 * CShort(NoWarpString = "--no-warp")))

        'TransferString = GetINIValue("TransferString")
        'UpdateTransferSelect()

        'AutoRefreshDir = GetINIValue("AutoRefreshDir")
        '.AutoRefreshDir.CheckState = Val(CStr(-1 * CShort(AutoRefreshDir)))

        'End With
        'FileClose(1)

        Exit Sub

LoadINIError:
        'FileClose(1)

        ' Prompt for new values if INI file isn't there or has a missing entry.
        'MsgBox("Please check your options.", MsgBoxStyle.OkOnly, "First Run")
        'OptionsForm.ShowDialog()

        Exit Sub
    End Sub


    'Write the INI file
    Public Sub SaveINI()
        '        On Error GoTo SaveINIError

        '        Dim DirTemp As String
        '        Dim FileName As String

        '        'Remember which directory we're in
        '        DirTemp = CurDir()

        '        'Save the values into the ini file
        '        FileClose(1)

        '        FileName = AddSlash(ExeDir) & INIFILE

        '        FileOpen(1, FileName, OpenMode.Output)

        '        With OptionsForm
        '            PutINIValue("StartingPath", AddSlash((.StartingPath.Text)))
        '            PutINIValue("DriveNumber", (.DriveNum.Text))
        '            PutINIValue("NoWarpString", g_strNoWarp)
        '            PutINIValue("TransferString", TransferString)
        '            PutINIValue("AutoRefreshDir", (.AutoRefreshDir.CheckState))
        '        End With

        '        FileClose(1)

        '        ' Return to previous directory
        '        ChDir(DirTemp)

        '        MsgBox("Preferences saved to " & FileName, MsgBoxStyle.Information)

        '        Exit Sub

        'SaveINIError:
        '        FileClose(1)
        '        MsgBox("SaveINI(): " & Err.Description & " (" & Err.Number & ")")
        '        Exit Sub
    End Sub
	
    '	Private Function GetINIValue(ByRef valname As String) As Object

    '		On Error GoTo GetINIValueError

    '		Dim temp As String
    '		Dim EqualLoc As Short
    '		Seek(1, 1)

    '		While Not EOF(1)
    '			temp = LineInput(1)
    '			If Left(temp, Len(valname)) = valname Then
    '				EqualLoc = InStr(temp, "=")

    '				If (EqualLoc = 0) Then
    '					MsgBox("Error: Corrupt line in INI file  " & temp)
    '					End
    '					Exit Function
    '				Else
    '					GetINIValue = Mid(temp, EqualLoc + 1)
    '					Exit Function
    '				End If
    '			End If
    '		End While

    '		'MsgBox "Error: INI file missing entry for " & valname
    '		'UPGRADE_WARNING: Use of Null/IsNull() detected. Click for more: 'ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?keyword="2EED02CB-5C0E-4DC1-AE94-4FAA3A30F51A"'
    '		'UPGRADE_WARNING: Couldn't resolve default property of object GetINIValue. Click for more: 'ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?keyword="6A50421D-15FE-4896-8A1B-2EC21E9037B2"'
    '		GetINIValue = System.DBNull.Value
    '		Exit Function

    'GetINIValueError: 
    '		FileClose(1)
    '		MsgBox("GetINIValueError(): " & Err.Description & " (" & Err.Number & ")")
    '		End
    '		Exit Function
    '	End Function
	
    'Private Sub PutINIValue(ByRef valname As String, ByRef value As Object)
    '	'UPGRADE_WARNING: Couldn't resolve default property of object value. Click for more: 'ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?keyword="6A50421D-15FE-4896-8A1B-2EC21E9037B2"'
    '	PrintLine(1, valname & "=" & CStr(value))
    'End Sub

    Public Function AddSlash(ByRef path As String) As String
        If Not (Right(path, 1) = "\") Then
            path = path & "\"
        End If

        AddSlash = path
    End Function
	
	
    'Private Sub UpdateTransferSelect()
    '	Select Case TransferString
    '		Case "original"
    '			OptionsForm.Transfer(0).Checked = True
    '		Case "serial1"
    '			OptionsForm.Transfer(1).Checked = True
    '		Case "serial2"
    '			OptionsForm.Transfer(2).Checked = True
    '		Case "parallel"
    '			OptionsForm.Transfer(3).Checked = True
    '		Case "auto"
    '			OptionsForm.Transfer(4).Checked = True
    '	End Select
    'End Sub
End Module