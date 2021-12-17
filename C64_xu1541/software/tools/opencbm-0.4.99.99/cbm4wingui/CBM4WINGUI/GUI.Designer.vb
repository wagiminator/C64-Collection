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
<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> Partial Class frmMain
#Region "Windows Form Designer generated code "
    <System.Diagnostics.DebuggerNonUserCode()> Public Sub New()
        MyBase.New()
        'This call is required by the Windows Form Designer.
        InitializeComponent()
    End Sub
    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> Protected Overloads Overrides Sub Dispose(ByVal Disposing As Boolean)
        If Disposing Then
            If Not components Is Nothing Then
                components.Dispose()
            End If
        End If
        MyBase.Dispose(Disposing)
    End Sub
    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer
    Public ToolTip1 As System.Windows.Forms.ToolTip
    Public WithEvents Options As System.Windows.Forms.Button
    Public WithEvents About As System.Windows.Forms.Button
    Public WithEvents CopyFromFloppy As System.Windows.Forms.Button
    Public WithEvents CopyToFloppy As System.Windows.Forms.Button
    Public WithEvents Label As Microsoft.VisualBasic.Compatibility.VB6.LabelArray
    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmMain))
        Me.ToolTip1 = New System.Windows.Forms.ToolTip(Me.components)
        Me.CopyFromFloppy = New System.Windows.Forms.Button
        Me.Options = New System.Windows.Forms.Button
        Me.About = New System.Windows.Forms.Button
        Me.CopyToFloppy = New System.Windows.Forms.Button
        Me.Label = New Microsoft.VisualBasic.Compatibility.VB6.LabelArray(Me.components)
        Me.SplitContainer1 = New System.Windows.Forms.SplitContainer
        Me.CBMDrive = New System.Windows.Forms.GroupBox
        Me.Drive = New System.Windows.Forms.ComboBox
        Me.CBMValidate = New System.Windows.Forms.Button
        Me.CBMInitialize = New System.Windows.Forms.Button
        Me.CBMFormat = New System.Windows.Forms.Button
        Me.CBMDirectory = New System.Windows.Forms.ListBox
        Me.CBMReset = New System.Windows.Forms.Button
        Me.CBMDriveStatus = New System.Windows.Forms.Button
        Me.CBMRefresh = New System.Windows.Forms.Button
        Me.CBMRename = New System.Windows.Forms.Button
        Me.CBMScratch = New System.Windows.Forms.Button
        Me._Label_0 = New System.Windows.Forms.Label
        Me.LastStatus = New System.Windows.Forms.Label
        Me._Label_5 = New System.Windows.Forms.Label
        Me.CBMDiskName = New System.Windows.Forms.Label
        Me.CBMDiskID = New System.Windows.Forms.Label
        Me._Label_3 = New System.Windows.Forms.Label
        Me._Label_4 = New System.Windows.Forms.Label
        Me.Frame2 = New System.Windows.Forms.GroupBox
        Me.PCDirectory = New FilesBrowser.FilesListBox
        Me.cmdBrowse = New System.Windows.Forms.Button
        Me.BlockText = New System.Windows.Forms.TextBox
        Me.KBText = New System.Windows.Forms.TextBox
        Me.MakeDir = New System.Windows.Forms.Button
        Me.PCRefresh = New System.Windows.Forms.Button
        Me.RunFile = New System.Windows.Forms.Button
        Me.PCRename = New System.Windows.Forms.Button
        Me.PCWorkingDir = New System.Windows.Forms.TextBox
        Me.PCDelete = New System.Windows.Forms.Button
        Me._Label_7 = New System.Windows.Forms.Label
        Me._Label_6 = New System.Windows.Forms.Label
        Me._Label_1 = New System.Windows.Forms.Label
        Me._Label_2 = New System.Windows.Forms.Label
        Me.LogGroup = New System.Windows.Forms.GroupBox
        Me.Log = New System.Windows.Forms.RichTextBox
        Me.folderBrowser = New System.Windows.Forms.FolderBrowserDialog
        CType(Me.Label, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SplitContainer1.Panel1.SuspendLayout()
        Me.SplitContainer1.Panel2.SuspendLayout()
        Me.SplitContainer1.SuspendLayout()
        Me.CBMDrive.SuspendLayout()
        Me.Frame2.SuspendLayout()
        Me.LogGroup.SuspendLayout()
        Me.SuspendLayout()
        '
        'CopyFromFloppy
        '
        resources.ApplyResources(Me.CopyFromFloppy, "CopyFromFloppy")
        Me.CopyFromFloppy.BackColor = System.Drawing.SystemColors.Control
        Me.CopyFromFloppy.Cursor = System.Windows.Forms.Cursors.Default
        Me.CopyFromFloppy.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CopyFromFloppy.Name = "CopyFromFloppy"
        Me.ToolTip1.SetToolTip(Me.CopyFromFloppy, resources.GetString("CopyFromFloppy.ToolTip"))
        Me.CopyFromFloppy.UseVisualStyleBackColor = True
        '
        'Options
        '
        resources.ApplyResources(Me.Options, "Options")
        Me.Options.BackColor = System.Drawing.SystemColors.Control
        Me.Options.Cursor = System.Windows.Forms.Cursors.Default
        Me.Options.ForeColor = System.Drawing.SystemColors.ControlText
        Me.Options.Name = "Options"
        Me.Options.UseVisualStyleBackColor = True
        '
        'About
        '
        resources.ApplyResources(Me.About, "About")
        Me.About.BackColor = System.Drawing.SystemColors.Control
        Me.About.Cursor = System.Windows.Forms.Cursors.Default
        Me.About.ForeColor = System.Drawing.SystemColors.ControlText
        Me.About.Name = "About"
        Me.About.UseVisualStyleBackColor = True
        '
        'CopyToFloppy
        '
        resources.ApplyResources(Me.CopyToFloppy, "CopyToFloppy")
        Me.CopyToFloppy.BackColor = System.Drawing.SystemColors.Control
        Me.CopyToFloppy.Cursor = System.Windows.Forms.Cursors.Default
        Me.CopyToFloppy.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CopyToFloppy.Name = "CopyToFloppy"
        Me.CopyToFloppy.UseVisualStyleBackColor = True
        '
        'SplitContainer1
        '
        resources.ApplyResources(Me.SplitContainer1, "SplitContainer1")
        Me.SplitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel2
        Me.SplitContainer1.Name = "SplitContainer1"
        '
        'SplitContainer1.Panel1
        '
        Me.SplitContainer1.Panel1.Controls.Add(Me.CBMDrive)
        Me.SplitContainer1.Panel1.Controls.Add(Me.CopyToFloppy)
        Me.SplitContainer1.Panel1.Controls.Add(Me.CopyFromFloppy)
        Me.SplitContainer1.Panel1.Controls.Add(Me.Frame2)
        '
        'SplitContainer1.Panel2
        '
        Me.SplitContainer1.Panel2.Controls.Add(Me.LogGroup)
        '
        'CBMDrive
        '
        resources.ApplyResources(Me.CBMDrive, "CBMDrive")
        Me.CBMDrive.BackColor = System.Drawing.SystemColors.Control
        Me.CBMDrive.Controls.Add(Me.Drive)
        Me.CBMDrive.Controls.Add(Me.CBMValidate)
        Me.CBMDrive.Controls.Add(Me.CBMInitialize)
        Me.CBMDrive.Controls.Add(Me.CBMFormat)
        Me.CBMDrive.Controls.Add(Me.CBMDirectory)
        Me.CBMDrive.Controls.Add(Me.CBMReset)
        Me.CBMDrive.Controls.Add(Me.CBMDriveStatus)
        Me.CBMDrive.Controls.Add(Me.CBMRefresh)
        Me.CBMDrive.Controls.Add(Me.CBMRename)
        Me.CBMDrive.Controls.Add(Me.CBMScratch)
        Me.CBMDrive.Controls.Add(Me._Label_0)
        Me.CBMDrive.Controls.Add(Me.LastStatus)
        Me.CBMDrive.Controls.Add(Me._Label_5)
        Me.CBMDrive.Controls.Add(Me.CBMDiskName)
        Me.CBMDrive.Controls.Add(Me.CBMDiskID)
        Me.CBMDrive.Controls.Add(Me._Label_3)
        Me.CBMDrive.Controls.Add(Me._Label_4)
        Me.CBMDrive.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMDrive.Name = "CBMDrive"
        Me.CBMDrive.TabStop = False
        '
        'Drive
        '
        Me.Drive.FormattingEnabled = True
        Me.Drive.Items.AddRange(New Object() {resources.GetString("Drive.Items"), resources.GetString("Drive.Items1"), resources.GetString("Drive.Items2"), resources.GetString("Drive.Items3")})
        resources.ApplyResources(Me.Drive, "Drive")
        Me.Drive.Name = "Drive"
        '
        'CBMValidate
        '
        Me.CBMValidate.BackColor = System.Drawing.SystemColors.Control
        Me.CBMValidate.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMValidate, "CBMValidate")
        Me.CBMValidate.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMValidate.Name = "CBMValidate"
        Me.CBMValidate.UseVisualStyleBackColor = True
        '
        'CBMInitialize
        '
        Me.CBMInitialize.BackColor = System.Drawing.SystemColors.Control
        Me.CBMInitialize.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMInitialize, "CBMInitialize")
        Me.CBMInitialize.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMInitialize.Name = "CBMInitialize"
        Me.CBMInitialize.UseVisualStyleBackColor = True
        '
        'CBMFormat
        '
        Me.CBMFormat.BackColor = System.Drawing.SystemColors.Control
        Me.CBMFormat.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMFormat, "CBMFormat")
        Me.CBMFormat.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMFormat.Name = "CBMFormat"
        Me.CBMFormat.UseVisualStyleBackColor = True
        '
        'CBMDirectory
        '
        resources.ApplyResources(Me.CBMDirectory, "CBMDirectory")
        Me.CBMDirectory.BackColor = System.Drawing.Color.FromArgb(CType(CType(0, Byte), Integer), CType(CType(0, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.CBMDirectory.Cursor = System.Windows.Forms.Cursors.Default
        Me.CBMDirectory.ForeColor = System.Drawing.Color.FromArgb(CType(CType(131, Byte), Integer), CType(CType(131, Byte), Integer), CType(CType(255, Byte), Integer))
        Me.CBMDirectory.Name = "CBMDirectory"
        Me.CBMDirectory.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended
        '
        'CBMReset
        '
        resources.ApplyResources(Me.CBMReset, "CBMReset")
        Me.CBMReset.BackColor = System.Drawing.SystemColors.Control
        Me.CBMReset.Cursor = System.Windows.Forms.Cursors.Default
        Me.CBMReset.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMReset.Name = "CBMReset"
        Me.CBMReset.UseVisualStyleBackColor = True
        '
        'CBMDriveStatus
        '
        resources.ApplyResources(Me.CBMDriveStatus, "CBMDriveStatus")
        Me.CBMDriveStatus.BackColor = System.Drawing.SystemColors.Control
        Me.CBMDriveStatus.Cursor = System.Windows.Forms.Cursors.Default
        Me.CBMDriveStatus.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMDriveStatus.Name = "CBMDriveStatus"
        Me.CBMDriveStatus.UseVisualStyleBackColor = True
        '
        'CBMRefresh
        '
        Me.CBMRefresh.BackColor = System.Drawing.SystemColors.Control
        Me.CBMRefresh.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMRefresh, "CBMRefresh")
        Me.CBMRefresh.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMRefresh.Name = "CBMRefresh"
        Me.CBMRefresh.UseVisualStyleBackColor = True
        '
        'CBMRename
        '
        Me.CBMRename.BackColor = System.Drawing.SystemColors.Control
        Me.CBMRename.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMRename, "CBMRename")
        Me.CBMRename.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMRename.Name = "CBMRename"
        Me.CBMRename.UseVisualStyleBackColor = True
        '
        'CBMScratch
        '
        Me.CBMScratch.BackColor = System.Drawing.SystemColors.Control
        Me.CBMScratch.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMScratch, "CBMScratch")
        Me.CBMScratch.ForeColor = System.Drawing.SystemColors.ControlText
        Me.CBMScratch.Name = "CBMScratch"
        Me.CBMScratch.UseVisualStyleBackColor = True
        '
        '_Label_0
        '
        resources.ApplyResources(Me._Label_0, "_Label_0")
        Me._Label_0.BackColor = System.Drawing.Color.Transparent
        Me._Label_0.Cursor = System.Windows.Forms.Cursors.Default
        Me._Label_0.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_0.Name = "_Label_0"
        '
        'LastStatus
        '
        resources.ApplyResources(Me.LastStatus, "LastStatus")
        Me.LastStatus.BackColor = System.Drawing.Color.FromArgb(CType(CType(0, Byte), Integer), CType(CType(0, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.LastStatus.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.LastStatus.Cursor = System.Windows.Forms.Cursors.Default
        Me.LastStatus.ForeColor = System.Drawing.Color.FromArgb(CType(CType(131, Byte), Integer), CType(CType(131, Byte), Integer), CType(CType(255, Byte), Integer))
        Me.LastStatus.Name = "LastStatus"
        '
        '_Label_5
        '
        resources.ApplyResources(Me._Label_5, "_Label_5")
        Me._Label_5.BackColor = System.Drawing.Color.Transparent
        Me._Label_5.Cursor = System.Windows.Forms.Cursors.Default
        Me._Label_5.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_5.Name = "_Label_5"
        '
        'CBMDiskName
        '
        Me.CBMDiskName.BackColor = System.Drawing.Color.FromArgb(CType(CType(131, Byte), Integer), CType(CType(131, Byte), Integer), CType(CType(255, Byte), Integer))
        Me.CBMDiskName.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.CBMDiskName.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMDiskName, "CBMDiskName")
        Me.CBMDiskName.ForeColor = System.Drawing.Color.FromArgb(CType(CType(0, Byte), Integer), CType(CType(0, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.CBMDiskName.Name = "CBMDiskName"
        '
        'CBMDiskID
        '
        Me.CBMDiskID.BackColor = System.Drawing.Color.FromArgb(CType(CType(131, Byte), Integer), CType(CType(131, Byte), Integer), CType(CType(255, Byte), Integer))
        Me.CBMDiskID.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.CBMDiskID.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me.CBMDiskID, "CBMDiskID")
        Me.CBMDiskID.ForeColor = System.Drawing.Color.FromArgb(CType(CType(0, Byte), Integer), CType(CType(0, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.CBMDiskID.Name = "CBMDiskID"
        '
        '_Label_3
        '
        Me._Label_3.BackColor = System.Drawing.Color.Transparent
        Me._Label_3.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me._Label_3, "_Label_3")
        Me._Label_3.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_3.Name = "_Label_3"
        '
        '_Label_4
        '
        Me._Label_4.BackColor = System.Drawing.Color.Transparent
        Me._Label_4.Cursor = System.Windows.Forms.Cursors.Default
        resources.ApplyResources(Me._Label_4, "_Label_4")
        Me._Label_4.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_4.Name = "_Label_4"
        '
        'Frame2
        '
        resources.ApplyResources(Me.Frame2, "Frame2")
        Me.Frame2.BackColor = System.Drawing.SystemColors.Control
        Me.Frame2.Controls.Add(Me.PCDirectory)
        Me.Frame2.Controls.Add(Me.cmdBrowse)
        Me.Frame2.Controls.Add(Me.Options)
        Me.Frame2.Controls.Add(Me.BlockText)
        Me.Frame2.Controls.Add(Me.KBText)
        Me.Frame2.Controls.Add(Me.About)
        Me.Frame2.Controls.Add(Me.MakeDir)
        Me.Frame2.Controls.Add(Me.PCRefresh)
        Me.Frame2.Controls.Add(Me.RunFile)
        Me.Frame2.Controls.Add(Me.PCRename)
        Me.Frame2.Controls.Add(Me.PCWorkingDir)
        Me.Frame2.Controls.Add(Me.PCDelete)
        Me.Frame2.Controls.Add(Me._Label_7)
        Me.Frame2.Controls.Add(Me._Label_6)
        Me.Frame2.Controls.Add(Me._Label_1)
        Me.Frame2.Controls.Add(Me._Label_2)
        Me.Frame2.ForeColor = System.Drawing.SystemColors.ControlText
        Me.Frame2.Name = "Frame2"
        Me.Frame2.TabStop = False
        '
        'PCDirectory
        '
        resources.ApplyResources(Me.PCDirectory, "PCDirectory")
        Me.PCDirectory.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed
        Me.PCDirectory.FileIconSize = FilesBrowser.IconSize.Small
        Me.PCDirectory.FormattingEnabled = True
        Me.PCDirectory.Name = "PCDirectory"
        Me.PCDirectory.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended
        '
        'cmdBrowse
        '
        resources.ApplyResources(Me.cmdBrowse, "cmdBrowse")
        Me.cmdBrowse.Name = "cmdBrowse"
        Me.cmdBrowse.UseVisualStyleBackColor = True
        '
        'BlockText
        '
        Me.BlockText.AcceptsReturn = True
        resources.ApplyResources(Me.BlockText, "BlockText")
        Me.BlockText.BackColor = System.Drawing.SystemColors.Window
        Me.BlockText.Cursor = System.Windows.Forms.Cursors.IBeam
        Me.BlockText.ForeColor = System.Drawing.SystemColors.WindowText
        Me.BlockText.Name = "BlockText"
        '
        'KBText
        '
        Me.KBText.AcceptsReturn = True
        resources.ApplyResources(Me.KBText, "KBText")
        Me.KBText.BackColor = System.Drawing.SystemColors.Window
        Me.KBText.Cursor = System.Windows.Forms.Cursors.IBeam
        Me.KBText.ForeColor = System.Drawing.SystemColors.WindowText
        Me.KBText.Name = "KBText"
        '
        'MakeDir
        '
        resources.ApplyResources(Me.MakeDir, "MakeDir")
        Me.MakeDir.BackColor = System.Drawing.SystemColors.Control
        Me.MakeDir.Cursor = System.Windows.Forms.Cursors.Default
        Me.MakeDir.ForeColor = System.Drawing.SystemColors.ControlText
        Me.MakeDir.Name = "MakeDir"
        Me.MakeDir.UseVisualStyleBackColor = True
        '
        'PCRefresh
        '
        resources.ApplyResources(Me.PCRefresh, "PCRefresh")
        Me.PCRefresh.BackColor = System.Drawing.SystemColors.Control
        Me.PCRefresh.Cursor = System.Windows.Forms.Cursors.Default
        Me.PCRefresh.ForeColor = System.Drawing.SystemColors.ControlText
        Me.PCRefresh.Name = "PCRefresh"
        Me.PCRefresh.UseVisualStyleBackColor = True
        '
        'RunFile
        '
        resources.ApplyResources(Me.RunFile, "RunFile")
        Me.RunFile.BackColor = System.Drawing.SystemColors.Control
        Me.RunFile.Cursor = System.Windows.Forms.Cursors.Default
        Me.RunFile.ForeColor = System.Drawing.SystemColors.ControlText
        Me.RunFile.Name = "RunFile"
        Me.RunFile.UseVisualStyleBackColor = True
        '
        'PCRename
        '
        resources.ApplyResources(Me.PCRename, "PCRename")
        Me.PCRename.BackColor = System.Drawing.SystemColors.Control
        Me.PCRename.Cursor = System.Windows.Forms.Cursors.Default
        Me.PCRename.ForeColor = System.Drawing.SystemColors.ControlText
        Me.PCRename.Name = "PCRename"
        Me.PCRename.UseVisualStyleBackColor = True
        '
        'PCWorkingDir
        '
        Me.PCWorkingDir.AcceptsReturn = True
        resources.ApplyResources(Me.PCWorkingDir, "PCWorkingDir")
        Me.PCWorkingDir.BackColor = System.Drawing.SystemColors.Window
        Me.PCWorkingDir.Cursor = System.Windows.Forms.Cursors.IBeam
        Me.PCWorkingDir.ForeColor = System.Drawing.SystemColors.WindowText
        Me.PCWorkingDir.Name = "PCWorkingDir"
        '
        'PCDelete
        '
        resources.ApplyResources(Me.PCDelete, "PCDelete")
        Me.PCDelete.BackColor = System.Drawing.SystemColors.Control
        Me.PCDelete.Cursor = System.Windows.Forms.Cursors.Default
        Me.PCDelete.ForeColor = System.Drawing.SystemColors.ControlText
        Me.PCDelete.Name = "PCDelete"
        Me.PCDelete.UseVisualStyleBackColor = True
        '
        '_Label_7
        '
        resources.ApplyResources(Me._Label_7, "_Label_7")
        Me._Label_7.BackColor = System.Drawing.Color.Transparent
        Me._Label_7.Cursor = System.Windows.Forms.Cursors.Default
        Me._Label_7.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_7.Name = "_Label_7"
        '
        '_Label_6
        '
        resources.ApplyResources(Me._Label_6, "_Label_6")
        Me._Label_6.BackColor = System.Drawing.Color.Transparent
        Me._Label_6.Cursor = System.Windows.Forms.Cursors.Default
        Me._Label_6.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_6.Name = "_Label_6"
        '
        '_Label_1
        '
        resources.ApplyResources(Me._Label_1, "_Label_1")
        Me._Label_1.BackColor = System.Drawing.Color.Transparent
        Me._Label_1.Cursor = System.Windows.Forms.Cursors.Default
        Me._Label_1.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_1.Name = "_Label_1"
        '
        '_Label_2
        '
        resources.ApplyResources(Me._Label_2, "_Label_2")
        Me._Label_2.BackColor = System.Drawing.Color.Transparent
        Me._Label_2.Cursor = System.Windows.Forms.Cursors.Default
        Me._Label_2.ForeColor = System.Drawing.SystemColors.ControlText
        Me._Label_2.Name = "_Label_2"
        '
        'LogGroup
        '
        Me.LogGroup.Controls.Add(Me.Log)
        resources.ApplyResources(Me.LogGroup, "LogGroup")
        Me.LogGroup.ForeColor = System.Drawing.SystemColors.ControlText
        Me.LogGroup.Name = "LogGroup"
        Me.LogGroup.TabStop = False
        '
        'Log
        '
        resources.ApplyResources(Me.Log, "Log")
        Me.Log.Name = "Log"
        Me.Log.ReadOnly = True
        '
        'folderBrowser
        '
        resources.ApplyResources(Me.folderBrowser, "folderBrowser")
        '
        'frmMain
        '
        resources.ApplyResources(Me, "$this")
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.BackColor = System.Drawing.SystemColors.Control
        Me.Controls.Add(Me.SplitContainer1)
        Me.Cursor = System.Windows.Forms.Cursors.Default
        Me.DoubleBuffered = True
        Me.Name = "frmMain"
        CType(Me.Label, System.ComponentModel.ISupportInitialize).EndInit()
        Me.SplitContainer1.Panel1.ResumeLayout(False)
        Me.SplitContainer1.Panel2.ResumeLayout(False)
        Me.SplitContainer1.ResumeLayout(False)
        Me.CBMDrive.ResumeLayout(False)
        Me.Frame2.ResumeLayout(False)
        Me.Frame2.PerformLayout()
        Me.LogGroup.ResumeLayout(False)
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents SplitContainer1 As System.Windows.Forms.SplitContainer
    Public WithEvents CBMDrive As System.Windows.Forms.GroupBox
    Public WithEvents CBMValidate As System.Windows.Forms.Button
    Public WithEvents CBMInitialize As System.Windows.Forms.Button
    Public WithEvents CBMFormat As System.Windows.Forms.Button
    Public WithEvents CBMDirectory As System.Windows.Forms.ListBox
    Public WithEvents CBMReset As System.Windows.Forms.Button
    Public WithEvents CBMDriveStatus As System.Windows.Forms.Button
    Public WithEvents CBMRefresh As System.Windows.Forms.Button
    Public WithEvents CBMRename As System.Windows.Forms.Button
    Public WithEvents CBMScratch As System.Windows.Forms.Button
    Public WithEvents _Label_0 As System.Windows.Forms.Label
    Public WithEvents LastStatus As System.Windows.Forms.Label
    Public WithEvents _Label_5 As System.Windows.Forms.Label
    Public WithEvents CBMDiskName As System.Windows.Forms.Label
    Public WithEvents CBMDiskID As System.Windows.Forms.Label
    Public WithEvents _Label_3 As System.Windows.Forms.Label
    Public WithEvents _Label_4 As System.Windows.Forms.Label
    Public WithEvents Frame2 As System.Windows.Forms.GroupBox
    Public WithEvents BlockText As System.Windows.Forms.TextBox
    Public WithEvents KBText As System.Windows.Forms.TextBox
    Public WithEvents MakeDir As System.Windows.Forms.Button
    Public WithEvents PCRefresh As System.Windows.Forms.Button
    Public WithEvents RunFile As System.Windows.Forms.Button
    Public WithEvents PCRename As System.Windows.Forms.Button
    Public WithEvents PCWorkingDir As System.Windows.Forms.TextBox
    Public WithEvents PCDelete As System.Windows.Forms.Button
    Public WithEvents _Label_7 As System.Windows.Forms.Label
    Public WithEvents _Label_6 As System.Windows.Forms.Label
    Public WithEvents _Label_1 As System.Windows.Forms.Label
    Public WithEvents _Label_2 As System.Windows.Forms.Label
    Friend WithEvents Drive As System.Windows.Forms.ComboBox
    Friend WithEvents LogGroup As System.Windows.Forms.GroupBox
    Friend WithEvents Log As System.Windows.Forms.RichTextBox
    Friend WithEvents cmdBrowse As System.Windows.Forms.Button
    Friend WithEvents folderBrowser As System.Windows.Forms.FolderBrowserDialog
    Friend WithEvents PCDirectory As FilesBrowser.FilesListBox
#End Region
End Class