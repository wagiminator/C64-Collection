Attribute VB_Name = "INIRoutines"
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

Const INIFILE = "gui4cbm4win.ini"

'Global Variables
Public ExeDir As String
Public DriveNumber As Integer
Public NoWarpString As String
Public TransferString As String
Public AutoRefreshDir As Boolean

'Load the INI file
Public Sub LoadINI()

On Error GoTo LoadINIError  'Get through as much as possible

    'Set Defaults
    OptionsForm.StartingPath.Text = CurDir
    OptionsForm.DriveNum.Text = "8"
    
    ' Let the "Disable-Warp" option unselected
    ' NoWarpString = "--no-warp"
    ' OptionsForm.CheckNoWarpMode.value = vbChecked
    
    OptionsForm.Transfer(4).value = True
    TransferString = "auto"
    
    AutoRefreshDir = True
    OptionsForm.AutoRefreshDir.value = vbChecked
    
    'Load the parameters from the ini file, overriding the defaults
    
    Open INIFILE For Input As #1
    With OptionsForm
        .StartingPath.Text = AddSlash(GetINIValue("StartingPath"))
                
        .DriveNum.Text = GetINIValue("DriveNumber")
        DriveNumber = .DriveNum.Text
        
        NoWarpString = GetINIValue("NoWarpString")
        .CheckNoWarpMode.value = Val(-1 * (NoWarpString = "--no-warp"))
        
        TransferString = GetINIValue("TransferString")
        UpdateTransferSelect
        
        AutoRefreshDir = GetINIValue("AutoRefreshDir")
        .AutoRefreshDir.value = Val(-1 * AutoRefreshDir)
        
    End With
    Close #1
    
    Exit Sub
    
LoadINIError:
    Close #1
    
    ' Prompt for new values if INI file isn't there or has a missing entry.
    MsgBox "Please check your options.", vbOKOnly, "First Run"
    OptionsForm.Show vbModal
    
    Exit Sub
End Sub


'Write the INI file
Public Sub SaveINI()
On Error GoTo SaveINIError:

    Dim DirTemp As String
    Dim FileName As String
    
    'Remember which directory we're in
    DirTemp = CurDir
    
    'Save the values into the ini file
    Close #1
    
    FileName = AddSlash(ExeDir) & INIFILE
    
    Open FileName For Output As #1
    
    With OptionsForm
        PutINIValue "StartingPath", AddSlash(.StartingPath.Text)
        PutINIValue "DriveNumber", .DriveNum.Text
        PutINIValue "NoWarpString", NoWarpString
        PutINIValue "TransferString", TransferString
        PutINIValue "AutoRefreshDir", .AutoRefreshDir.value
    End With
    
    Close #1
    
    ' Return to previous directory
    ChDir DirTemp
    
    MsgBox "Preferences saved to " & FileName, vbInformation
    
    Exit Sub

SaveINIError:
    Close #1
    MsgBox "SaveINI(): " & Err.Description & " (" & Err.Number & ")"
    Exit Sub
End Sub

Private Function GetINIValue(valname As String) As Variant

On Error GoTo GetINIValueError:

    Dim temp As String
    Dim EqualLoc As Integer
    Seek #1, 1
  
    While Not EOF(1)
        Line Input #1, temp
        If Left$(temp, Len(valname)) = valname Then
            EqualLoc = InStr(temp, "=")
            
            If (EqualLoc = 0) Then
                MsgBox "Error: Corrupt line in INI file  " & temp
                End
                Exit Function
             Else
                GetINIValue = Mid$(temp, EqualLoc + 1)
                Exit Function
            End If
        End If
    Wend
    
    'MsgBox "Error: INI file missing entry for " & valname
    GetINIValue = Null
    Exit Function
    
GetINIValueError:
    Close #1
    MsgBox "GetINIValueError(): " & Err.Description & " (" & Err.Number & ")"
    End
    Exit Function
End Function

Private Sub PutINIValue(valname As String, value As Variant)
    Print #1, valname & "=" & CStr(value)
End Sub

Public Function AddSlash(path As String) As String
    If Not (Right$(path, 1) = "\") Then
            path = path & "\"
    End If
    
    AddSlash = path
End Function


Private Sub UpdateTransferSelect()
      Select Case TransferString
        Case "original":
            OptionsForm.Transfer(0).value = True
        Case "serial1":
            OptionsForm.Transfer(1).value = True
        Case "serial2":
            OptionsForm.Transfer(2).value = True
        Case "parallel":
            OptionsForm.Transfer(3).value = True
        Case "auto":
            OptionsForm.Transfer(4).value = True
    End Select
End Sub
