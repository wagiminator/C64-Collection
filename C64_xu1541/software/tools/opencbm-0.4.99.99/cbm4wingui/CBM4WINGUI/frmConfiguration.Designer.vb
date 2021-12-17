<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmConfiguration
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmConfiguration))
        Me.m_objTools = New Pabo.MozBar.MozPane
        Me.m_objImages = New System.Windows.Forms.ImageList(Me.components)
        Me.m_objToolCBMConfig = New Pabo.MozBar.MozItem
        Me.m_objToolPCConfig = New Pabo.MozBar.MozItem
        Me.m_objToolEmulatorConfig = New Pabo.MozBar.MozItem
        Me.m_objButtonPanel = New System.Windows.Forms.Panel
        Me.m_btnHelp = New System.Windows.Forms.Button
        Me.m_btnCancel = New System.Windows.Forms.Button
        Me.m_btnDone = New System.Windows.Forms.Button
        Me.m_objTabs = New System.Windows.Forms.TabControl
        Me.m_objPageOpenCBM = New System.Windows.Forms.TabPage
        Me.m_objGroupOpenCBM = New System.Windows.Forms.GroupBox
        Me.m_objGroupTransferMode = New System.Windows.Forms.GroupBox
        Me.m_radParallel = New System.Windows.Forms.RadioButton
        Me.m_radSerial2 = New System.Windows.Forms.RadioButton
        Me.m_radAutomatic = New System.Windows.Forms.RadioButton
        Me.m_radSerial1 = New System.Windows.Forms.RadioButton
        Me.m_radOriginal = New System.Windows.Forms.RadioButton
        Me.m_chkRefreshDirectory = New System.Windows.Forms.CheckBox
        Me.m_chkPreviewCommands = New System.Windows.Forms.CheckBox
        Me.m_chkDisableWarp = New System.Windows.Forms.CheckBox
        Me.m_btnResetBus = New System.Windows.Forms.Button
        Me.m_btnDetectDevices = New System.Windows.Forms.Button
        Me.m_objDevicesGrid = New System.Windows.Forms.DataGridView
        Me.DefaultDataGridViewCheckBoxColumn = New System.Windows.Forms.DataGridViewCheckBoxColumn
        Me.DeviceDataGridViewTextBoxColumn = New System.Windows.Forms.DataGridViewTextBoxColumn
        Me.ModelDataGridViewTextBoxColumn = New System.Windows.Forms.DataGridViewTextBoxColumn
        Me.m_objCbmDevicesBindingSource1 = New System.Windows.Forms.BindingSource(Me.components)
        Me.m_objCbmDevicesBindingSource = New System.Windows.Forms.BindingSource(Me.components)
        Me.m_objCbmDevices = New GUI4CBM4WIN.CbmDevices
        Me.Label1 = New System.Windows.Forms.Label
        Me.m_objPagePC = New System.Windows.Forms.TabPage
        Me.m_objGroupPC = New System.Windows.Forms.GroupBox
        Me.m_btnBrowse = New System.Windows.Forms.Button
        Me.m_txtInitialDirectory = New System.Windows.Forms.TextBox
        Me.m_lblInitialDirectory = New System.Windows.Forms.Label
        Me.m_objPageEmulator = New System.Windows.Forms.TabPage
        Me.m_objGroupEmulator = New System.Windows.Forms.GroupBox
        Me.m_lblComingSoon = New System.Windows.Forms.Label
        Me.m_objFolder = New System.Windows.Forms.FolderBrowserDialog
        Me.m_objHelp = New System.Windows.Forms.HelpProvider
        CType(Me.m_objTools, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.m_objTools.SuspendLayout()
        Me.m_objButtonPanel.SuspendLayout()
        Me.m_objTabs.SuspendLayout()
        Me.m_objPageOpenCBM.SuspendLayout()
        Me.m_objGroupOpenCBM.SuspendLayout()
        Me.m_objGroupTransferMode.SuspendLayout()
        CType(Me.m_objDevicesGrid, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.m_objCbmDevicesBindingSource1, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.m_objCbmDevicesBindingSource, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.m_objCbmDevices, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.m_objPagePC.SuspendLayout()
        Me.m_objGroupPC.SuspendLayout()
        Me.m_objPageEmulator.SuspendLayout()
        Me.m_objGroupEmulator.SuspendLayout()
        Me.SuspendLayout()
        '
        'm_objTools
        '
        Me.m_objTools.BorderColor = System.Drawing.Color.FromArgb(CType(CType(197, Byte), Integer), CType(CType(198, Byte), Integer), CType(CType(214, Byte), Integer))
        resources.ApplyResources(Me.m_objTools, "m_objTools")
        Me.m_objTools.ImageList = Me.m_objImages
        Me.m_objTools.ItemColors.Divider = System.Drawing.Color.FromArgb(CType(CType(197, Byte), Integer), CType(CType(198, Byte), Integer), CType(CType(214, Byte), Integer))
        Me.m_objTools.ItemColors.FocusBackground = System.Drawing.Color.FromArgb(CType(CType(237, Byte), Integer), CType(CType(237, Byte), Integer), CType(CType(241, Byte), Integer))
        Me.m_objTools.ItemColors.FocusBorder = System.Drawing.Color.FromArgb(CType(CType(217, Byte), Integer), CType(CType(218, Byte), Integer), CType(CType(227, Byte), Integer))
        Me.m_objTools.ItemColors.SelectedBackground = System.Drawing.Color.FromArgb(CType(CType(217, Byte), Integer), CType(CType(218, Byte), Integer), CType(CType(227, Byte), Integer))
        Me.m_objTools.ItemColors.SelectedBorder = System.Drawing.Color.FromArgb(CType(CType(101, Byte), Integer), CType(CType(104, Byte), Integer), CType(CType(137, Byte), Integer))
        Me.m_objTools.Items.AddRange(New Pabo.MozBar.MozItem() {Me.m_objToolCBMConfig, Me.m_objToolPCConfig, Me.m_objToolEmulatorConfig})
        Me.m_objTools.Name = "m_objTools"
        Me.m_objTools.SelectButton = Pabo.MozBar.MozSelectButton.Any
        '
        'm_objImages
        '
        Me.m_objImages.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit
        resources.ApplyResources(Me.m_objImages, "m_objImages")
        Me.m_objImages.TransparentColor = System.Drawing.Color.Transparent
        '
        'm_objToolCBMConfig
        '
        Me.m_objToolCBMConfig.Images.Focus = -1
        Me.m_objToolCBMConfig.Images.FocusImage = Nothing
        Me.m_objToolCBMConfig.Images.Normal = -1
        Me.m_objToolCBMConfig.Images.NormalImage = Nothing
        Me.m_objToolCBMConfig.Images.Selected = -1
        Me.m_objToolCBMConfig.Images.SelectedImage = Nothing
        Me.m_objToolCBMConfig.ItemStyle = Pabo.MozBar.MozItemStyle.Text
        resources.ApplyResources(Me.m_objToolCBMConfig, "m_objToolCBMConfig")
        Me.m_objToolCBMConfig.Name = "m_objToolCBMConfig"
        Me.m_objToolCBMConfig.Tag = "OpenCBM"
        '
        'm_objToolPCConfig
        '
        Me.m_objToolPCConfig.Images.Focus = -1
        Me.m_objToolPCConfig.Images.FocusImage = Nothing
        Me.m_objToolPCConfig.Images.Normal = -1
        Me.m_objToolPCConfig.Images.NormalImage = Nothing
        Me.m_objToolPCConfig.Images.Selected = -1
        Me.m_objToolPCConfig.Images.SelectedImage = Nothing
        Me.m_objToolPCConfig.ItemStyle = Pabo.MozBar.MozItemStyle.Text
        resources.ApplyResources(Me.m_objToolPCConfig, "m_objToolPCConfig")
        Me.m_objToolPCConfig.Name = "m_objToolPCConfig"
        Me.m_objToolPCConfig.Tag = "PC"
        '
        'm_objToolEmulatorConfig
        '
        Me.m_objToolEmulatorConfig.Images.Focus = -1
        Me.m_objToolEmulatorConfig.Images.FocusImage = Nothing
        Me.m_objToolEmulatorConfig.Images.Normal = -1
        Me.m_objToolEmulatorConfig.Images.NormalImage = Nothing
        Me.m_objToolEmulatorConfig.Images.Selected = -1
        Me.m_objToolEmulatorConfig.Images.SelectedImage = Nothing
        Me.m_objToolEmulatorConfig.ItemStyle = Pabo.MozBar.MozItemStyle.Text
        resources.ApplyResources(Me.m_objToolEmulatorConfig, "m_objToolEmulatorConfig")
        Me.m_objToolEmulatorConfig.Name = "m_objToolEmulatorConfig"
        Me.m_objToolEmulatorConfig.Tag = "Emulator"
        '
        'm_objButtonPanel
        '
        Me.m_objButtonPanel.Controls.Add(Me.m_btnHelp)
        Me.m_objButtonPanel.Controls.Add(Me.m_btnCancel)
        Me.m_objButtonPanel.Controls.Add(Me.m_btnDone)
        resources.ApplyResources(Me.m_objButtonPanel, "m_objButtonPanel")
        Me.m_objButtonPanel.Name = "m_objButtonPanel"
        '
        'm_btnHelp
        '
        resources.ApplyResources(Me.m_btnHelp, "m_btnHelp")
        Me.m_btnHelp.Name = "m_btnHelp"
        Me.m_btnHelp.UseVisualStyleBackColor = True
        '
        'm_btnCancel
        '
        resources.ApplyResources(Me.m_btnCancel, "m_btnCancel")
        Me.m_btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.m_btnCancel.Name = "m_btnCancel"
        Me.m_btnCancel.UseVisualStyleBackColor = True
        '
        'm_btnDone
        '
        resources.ApplyResources(Me.m_btnDone, "m_btnDone")
        Me.m_btnDone.Name = "m_btnDone"
        Me.m_btnDone.UseVisualStyleBackColor = True
        '
        'm_objTabs
        '
        resources.ApplyResources(Me.m_objTabs, "m_objTabs")
        Me.m_objTabs.Controls.Add(Me.m_objPageOpenCBM)
        Me.m_objTabs.Controls.Add(Me.m_objPagePC)
        Me.m_objTabs.Controls.Add(Me.m_objPageEmulator)
        Me.m_objTabs.Name = "m_objTabs"
        Me.m_objTabs.SelectedIndex = 0
        '
        'm_objPageOpenCBM
        '
        Me.m_objPageOpenCBM.Controls.Add(Me.m_objGroupOpenCBM)
        resources.ApplyResources(Me.m_objPageOpenCBM, "m_objPageOpenCBM")
        Me.m_objPageOpenCBM.Name = "m_objPageOpenCBM"
        Me.m_objPageOpenCBM.UseVisualStyleBackColor = True
        '
        'm_objGroupOpenCBM
        '
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_objGroupTransferMode)
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_chkRefreshDirectory)
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_chkPreviewCommands)
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_chkDisableWarp)
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_btnResetBus)
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_btnDetectDevices)
        Me.m_objGroupOpenCBM.Controls.Add(Me.m_objDevicesGrid)
        Me.m_objGroupOpenCBM.Controls.Add(Me.Label1)
        resources.ApplyResources(Me.m_objGroupOpenCBM, "m_objGroupOpenCBM")
        Me.m_objGroupOpenCBM.Name = "m_objGroupOpenCBM"
        Me.m_objGroupOpenCBM.TabStop = False
        '
        'm_objGroupTransferMode
        '
        Me.m_objGroupTransferMode.Controls.Add(Me.m_radParallel)
        Me.m_objGroupTransferMode.Controls.Add(Me.m_radSerial2)
        Me.m_objGroupTransferMode.Controls.Add(Me.m_radAutomatic)
        Me.m_objGroupTransferMode.Controls.Add(Me.m_radSerial1)
        Me.m_objGroupTransferMode.Controls.Add(Me.m_radOriginal)
        resources.ApplyResources(Me.m_objGroupTransferMode, "m_objGroupTransferMode")
        Me.m_objGroupTransferMode.Name = "m_objGroupTransferMode"
        Me.m_objGroupTransferMode.TabStop = False
        '
        'm_radParallel
        '
        resources.ApplyResources(Me.m_radParallel, "m_radParallel")
        Me.m_radParallel.Name = "m_radParallel"
        Me.m_radParallel.TabStop = True
        Me.m_radParallel.Tag = "parallel"
        Me.m_radParallel.UseVisualStyleBackColor = True
        '
        'm_radSerial2
        '
        resources.ApplyResources(Me.m_radSerial2, "m_radSerial2")
        Me.m_radSerial2.Name = "m_radSerial2"
        Me.m_radSerial2.TabStop = True
        Me.m_radSerial2.Tag = "serial2"
        Me.m_radSerial2.UseVisualStyleBackColor = True
        '
        'm_radAutomatic
        '
        resources.ApplyResources(Me.m_radAutomatic, "m_radAutomatic")
        Me.m_radAutomatic.Checked = True
        Me.m_radAutomatic.Name = "m_radAutomatic"
        Me.m_radAutomatic.TabStop = True
        Me.m_radAutomatic.Tag = "auto"
        Me.m_radAutomatic.UseVisualStyleBackColor = True
        '
        'm_radSerial1
        '
        resources.ApplyResources(Me.m_radSerial1, "m_radSerial1")
        Me.m_radSerial1.Name = "m_radSerial1"
        Me.m_radSerial1.Tag = "serial1"
        Me.m_radSerial1.UseVisualStyleBackColor = True
        '
        'm_radOriginal
        '
        resources.ApplyResources(Me.m_radOriginal, "m_radOriginal")
        Me.m_radOriginal.Name = "m_radOriginal"
        Me.m_radOriginal.Tag = "original"
        Me.m_radOriginal.UseVisualStyleBackColor = True
        '
        'm_chkRefreshDirectory
        '
        resources.ApplyResources(Me.m_chkRefreshDirectory, "m_chkRefreshDirectory")
        Me.m_chkRefreshDirectory.Checked = True
        Me.m_chkRefreshDirectory.CheckState = System.Windows.Forms.CheckState.Checked
        Me.m_chkRefreshDirectory.Name = "m_chkRefreshDirectory"
        Me.m_chkRefreshDirectory.UseVisualStyleBackColor = True
        '
        'm_chkPreviewCommands
        '
        resources.ApplyResources(Me.m_chkPreviewCommands, "m_chkPreviewCommands")
        Me.m_chkPreviewCommands.Name = "m_chkPreviewCommands"
        Me.m_chkPreviewCommands.UseVisualStyleBackColor = True
        '
        'm_chkDisableWarp
        '
        resources.ApplyResources(Me.m_chkDisableWarp, "m_chkDisableWarp")
        Me.m_chkDisableWarp.Name = "m_chkDisableWarp"
        Me.m_chkDisableWarp.UseVisualStyleBackColor = True
        '
        'm_btnResetBus
        '
        resources.ApplyResources(Me.m_btnResetBus, "m_btnResetBus")
        Me.m_btnResetBus.Name = "m_btnResetBus"
        Me.m_btnResetBus.UseVisualStyleBackColor = True
        '
        'm_btnDetectDevices
        '
        resources.ApplyResources(Me.m_btnDetectDevices, "m_btnDetectDevices")
        Me.m_btnDetectDevices.Name = "m_btnDetectDevices"
        Me.m_btnDetectDevices.UseVisualStyleBackColor = True
        '
        'm_objDevicesGrid
        '
        Me.m_objDevicesGrid.AllowUserToAddRows = False
        Me.m_objDevicesGrid.AllowUserToDeleteRows = False
        Me.m_objDevicesGrid.AutoGenerateColumns = False
        Me.m_objDevicesGrid.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize
        Me.m_objDevicesGrid.Columns.AddRange(New System.Windows.Forms.DataGridViewColumn() {Me.DefaultDataGridViewCheckBoxColumn, Me.DeviceDataGridViewTextBoxColumn, Me.ModelDataGridViewTextBoxColumn})
        Me.m_objDevicesGrid.DataSource = Me.m_objCbmDevicesBindingSource1
        resources.ApplyResources(Me.m_objDevicesGrid, "m_objDevicesGrid")
        Me.m_objDevicesGrid.Name = "m_objDevicesGrid"
        Me.m_objDevicesGrid.RowTemplate.Height = 24
        '
        'DefaultDataGridViewCheckBoxColumn
        '
        Me.DefaultDataGridViewCheckBoxColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.AllCells
        Me.DefaultDataGridViewCheckBoxColumn.DataPropertyName = "Default"
        resources.ApplyResources(Me.DefaultDataGridViewCheckBoxColumn, "DefaultDataGridViewCheckBoxColumn")
        Me.DefaultDataGridViewCheckBoxColumn.Name = "DefaultDataGridViewCheckBoxColumn"
        '
        'DeviceDataGridViewTextBoxColumn
        '
        Me.DeviceDataGridViewTextBoxColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.AllCells
        Me.DeviceDataGridViewTextBoxColumn.DataPropertyName = "Device"
        resources.ApplyResources(Me.DeviceDataGridViewTextBoxColumn, "DeviceDataGridViewTextBoxColumn")
        Me.DeviceDataGridViewTextBoxColumn.Name = "DeviceDataGridViewTextBoxColumn"
        Me.DeviceDataGridViewTextBoxColumn.ReadOnly = True
        '
        'ModelDataGridViewTextBoxColumn
        '
        Me.ModelDataGridViewTextBoxColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.AllCells
        Me.ModelDataGridViewTextBoxColumn.DataPropertyName = "Model"
        resources.ApplyResources(Me.ModelDataGridViewTextBoxColumn, "ModelDataGridViewTextBoxColumn")
        Me.ModelDataGridViewTextBoxColumn.Name = "ModelDataGridViewTextBoxColumn"
        Me.ModelDataGridViewTextBoxColumn.ReadOnly = True
        '
        'm_objCbmDevicesBindingSource1
        '
        Me.m_objCbmDevicesBindingSource1.DataMember = "CbmDevices"
        Me.m_objCbmDevicesBindingSource1.DataSource = Me.m_objCbmDevicesBindingSource
        '
        'm_objCbmDevicesBindingSource
        '
        Me.m_objCbmDevicesBindingSource.DataSource = Me.m_objCbmDevices
        Me.m_objCbmDevicesBindingSource.Position = 0
        '
        'm_objCbmDevices
        '
        Me.m_objCbmDevices.DataSetName = "CbmDevices"
        Me.m_objCbmDevices.SchemaSerializationMode = System.Data.SchemaSerializationMode.IncludeSchema
        '
        'Label1
        '
        resources.ApplyResources(Me.Label1, "Label1")
        Me.Label1.Name = "Label1"
        '
        'm_objPagePC
        '
        Me.m_objPagePC.Controls.Add(Me.m_objGroupPC)
        resources.ApplyResources(Me.m_objPagePC, "m_objPagePC")
        Me.m_objPagePC.Name = "m_objPagePC"
        Me.m_objPagePC.UseVisualStyleBackColor = True
        '
        'm_objGroupPC
        '
        Me.m_objGroupPC.Controls.Add(Me.m_btnBrowse)
        Me.m_objGroupPC.Controls.Add(Me.m_txtInitialDirectory)
        Me.m_objGroupPC.Controls.Add(Me.m_lblInitialDirectory)
        resources.ApplyResources(Me.m_objGroupPC, "m_objGroupPC")
        Me.m_objGroupPC.Name = "m_objGroupPC"
        Me.m_objGroupPC.TabStop = False
        '
        'm_btnBrowse
        '
        resources.ApplyResources(Me.m_btnBrowse, "m_btnBrowse")
        Me.m_btnBrowse.Name = "m_btnBrowse"
        Me.m_btnBrowse.UseVisualStyleBackColor = True
        '
        'm_txtInitialDirectory
        '
        resources.ApplyResources(Me.m_txtInitialDirectory, "m_txtInitialDirectory")
        Me.m_txtInitialDirectory.Name = "m_txtInitialDirectory"
        '
        'm_lblInitialDirectory
        '
        resources.ApplyResources(Me.m_lblInitialDirectory, "m_lblInitialDirectory")
        Me.m_lblInitialDirectory.Name = "m_lblInitialDirectory"
        '
        'm_objPageEmulator
        '
        Me.m_objPageEmulator.Controls.Add(Me.m_objGroupEmulator)
        resources.ApplyResources(Me.m_objPageEmulator, "m_objPageEmulator")
        Me.m_objPageEmulator.Name = "m_objPageEmulator"
        Me.m_objPageEmulator.UseVisualStyleBackColor = True
        '
        'm_objGroupEmulator
        '
        Me.m_objGroupEmulator.Controls.Add(Me.m_lblComingSoon)
        resources.ApplyResources(Me.m_objGroupEmulator, "m_objGroupEmulator")
        Me.m_objGroupEmulator.Name = "m_objGroupEmulator"
        Me.m_objGroupEmulator.TabStop = False
        '
        'm_lblComingSoon
        '
        resources.ApplyResources(Me.m_lblComingSoon, "m_lblComingSoon")
        Me.m_lblComingSoon.Name = "m_lblComingSoon"
        '
        'm_objFolder
        '
        resources.ApplyResources(Me.m_objFolder, "m_objFolder")
        '
        'm_objHelp
        '
        resources.ApplyResources(Me.m_objHelp, "m_objHelp")
        '
        'frmConfiguration
        '
        Me.AcceptButton = Me.m_btnDone
        resources.ApplyResources(Me, "$this")
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.CancelButton = Me.m_btnCancel
        Me.Controls.Add(Me.m_objButtonPanel)
        Me.Controls.Add(Me.m_objTabs)
        Me.Controls.Add(Me.m_objTools)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "frmConfiguration"
        Me.ShowInTaskbar = False
        CType(Me.m_objTools, System.ComponentModel.ISupportInitialize).EndInit()
        Me.m_objTools.ResumeLayout(False)
        Me.m_objButtonPanel.ResumeLayout(False)
        Me.m_objTabs.ResumeLayout(False)
        Me.m_objPageOpenCBM.ResumeLayout(False)
        Me.m_objGroupOpenCBM.ResumeLayout(False)
        Me.m_objGroupOpenCBM.PerformLayout()
        Me.m_objGroupTransferMode.ResumeLayout(False)
        Me.m_objGroupTransferMode.PerformLayout()
        CType(Me.m_objDevicesGrid, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.m_objCbmDevicesBindingSource1, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.m_objCbmDevicesBindingSource, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.m_objCbmDevices, System.ComponentModel.ISupportInitialize).EndInit()
        Me.m_objPagePC.ResumeLayout(False)
        Me.m_objGroupPC.ResumeLayout(False)
        Me.m_objGroupPC.PerformLayout()
        Me.m_objPageEmulator.ResumeLayout(False)
        Me.m_objGroupEmulator.ResumeLayout(False)
        Me.m_objGroupEmulator.PerformLayout()
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents m_objTools As Pabo.MozBar.MozPane
    Friend WithEvents m_objToolPCConfig As Pabo.MozBar.MozItem
    Friend WithEvents m_objToolCBMConfig As Pabo.MozBar.MozItem
    Friend WithEvents m_objToolEmulatorConfig As Pabo.MozBar.MozItem
    Friend WithEvents m_objImages As System.Windows.Forms.ImageList
    Friend WithEvents m_objButtonPanel As System.Windows.Forms.Panel
    Friend WithEvents m_btnDone As System.Windows.Forms.Button
    Friend WithEvents m_btnCancel As System.Windows.Forms.Button
    Friend WithEvents m_objTabs As System.Windows.Forms.TabControl
    Friend WithEvents m_objPagePC As System.Windows.Forms.TabPage
    Friend WithEvents m_objPageOpenCBM As System.Windows.Forms.TabPage
    Friend WithEvents m_objPageEmulator As System.Windows.Forms.TabPage
    Friend WithEvents m_objGroupPC As System.Windows.Forms.GroupBox
    Friend WithEvents m_objGroupOpenCBM As System.Windows.Forms.GroupBox
    Friend WithEvents m_objGroupEmulator As System.Windows.Forms.GroupBox
    Friend WithEvents m_lblComingSoon As System.Windows.Forms.Label
    Friend WithEvents m_btnHelp As System.Windows.Forms.Button
    Friend WithEvents m_objFolder As System.Windows.Forms.FolderBrowserDialog
    Friend WithEvents m_objHelp As System.Windows.Forms.HelpProvider
    Friend WithEvents m_btnBrowse As System.Windows.Forms.Button
    Friend WithEvents m_txtInitialDirectory As System.Windows.Forms.TextBox
    Friend WithEvents m_lblInitialDirectory As System.Windows.Forms.Label
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents m_btnDetectDevices As System.Windows.Forms.Button
    Friend WithEvents m_objDevicesGrid As System.Windows.Forms.DataGridView
    Friend WithEvents m_objCbmDevicesBindingSource1 As System.Windows.Forms.BindingSource
    Friend WithEvents m_objCbmDevicesBindingSource As System.Windows.Forms.BindingSource
    Friend WithEvents m_btnResetBus As System.Windows.Forms.Button
    Friend WithEvents m_chkDisableWarp As System.Windows.Forms.CheckBox
    Friend WithEvents m_chkRefreshDirectory As System.Windows.Forms.CheckBox
    Friend WithEvents m_chkPreviewCommands As System.Windows.Forms.CheckBox
    Friend WithEvents m_objGroupTransferMode As System.Windows.Forms.GroupBox
    Friend WithEvents m_radParallel As System.Windows.Forms.RadioButton
    Friend WithEvents m_radSerial2 As System.Windows.Forms.RadioButton
    Friend WithEvents m_radAutomatic As System.Windows.Forms.RadioButton
    Friend WithEvents m_radSerial1 As System.Windows.Forms.RadioButton
    Friend WithEvents m_radOriginal As System.Windows.Forms.RadioButton
    Friend WithEvents DefaultDataGridViewCheckBoxColumn As System.Windows.Forms.DataGridViewCheckBoxColumn
    Friend WithEvents DeviceDataGridViewTextBoxColumn As System.Windows.Forms.DataGridViewTextBoxColumn
    Friend WithEvents ModelDataGridViewTextBoxColumn As System.Windows.Forms.DataGridViewTextBoxColumn
    Public WithEvents m_objCbmDevices As GUI4CBM4WIN.CbmDevices
End Class
