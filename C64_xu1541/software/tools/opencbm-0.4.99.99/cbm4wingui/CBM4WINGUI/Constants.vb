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
Imports System.Configuration
Imports System.Runtime.InteropServices
Imports System.Text

Module Constants_Renamed
    Public Declare Function ShellExecute Lib "shell32.dll" Alias "ShellExecuteA" (ByVal hWnd As Integer, ByVal lpOperation As String, ByVal lpFile As String, ByVal lpParameters As String, ByVal lpDirectory As String, ByVal nShowCmd As Integer) As Integer

    Public Declare Sub SetWindowPos Lib "user32" (ByVal hWnd As Integer, ByVal hWndInsertAfter As Integer, ByVal x As Integer, ByVal y As Integer, ByVal cx As Integer, ByVal cy As Integer, ByVal wFlags As Integer)
    Private IsOnTop As Boolean

    Public Const HWND_TOPMOST As Short = -&H1S
    Public Const HWND_NOTOPMOST As Short = -&H2S
    Public Const SWP_NOSIZE As Short = &H1S
    Public Const SWP_NOMOVE As Short = &H2S

    Public CANCELSTRING As String = "***CANCEL***"

    Public Structure ReturnStringType
        Dim Output As String
        Dim Errors As String
    End Structure

    Private g_objConfig As Configuration = _
        ConfigurationManager.OpenExeConfiguration(ConfigurationUserLevel.None)

    Public ReadOnly Property Config() As Configuration
        Get
            Return g_objConfig
        End Get
    End Property
End Module