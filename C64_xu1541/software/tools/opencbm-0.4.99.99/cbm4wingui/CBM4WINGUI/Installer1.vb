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

Imports System.ComponentModel
Imports System.Configuration.Install
Imports System.Threading

Public Class Installer1

    Public Sub New()
        MyBase.New()

        'This call is required by the Component Designer.
        InitializeComponent()

        'Add initialization code after the call to InitializeComponent

    End Sub

    Private Sub Installer1_AfterInstall(ByVal sender As Object, ByVal e As System.Configuration.Install.InstallEventArgs) Handles Me.AfterInstall
    End Sub

    Private Sub Installer1_BeforeRollback(ByVal sender As Object, ByVal e As System.Configuration.Install.InstallEventArgs) Handles Me.BeforeRollback
        Execute("--remove")
    End Sub

    Private Sub Installer1_BeforeUninstall(ByVal sender As Object, ByVal e As System.Configuration.Install.InstallEventArgs) Handles Me.BeforeUninstall
        Execute("--remove")
    End Sub

    Private Sub Execute(ByVal args As String)
        Dim p As Process
        Dim path As String = System.IO.Path.GetDirectoryName(Context.Parameters("assemblypath"))
        Dim c As String = System.IO.Path.Combine(path, "instcbm.exe")
        Dim oldPath As String = Environment.CurrentDirectory

        Environment.CurrentDirectory = path
        p = Process.Start(c, args)
        p.WaitForExit()
        Environment.CurrentDirectory = oldPath
    End Sub

    Private Sub Installer1_Committing(ByVal sender As Object, ByVal e As System.Configuration.Install.InstallEventArgs) Handles Me.Committing
        Execute("--automatic")
    End Sub
End Class
