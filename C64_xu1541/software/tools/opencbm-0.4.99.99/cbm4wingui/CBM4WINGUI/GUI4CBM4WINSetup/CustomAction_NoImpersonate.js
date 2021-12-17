// NoImpersonate.js <msi-file>
// Performs a post-build fixup of an msi to change all deferred custom actions to NoImpersonate

// Constant values from Windows Installer
var msiOpenDatabaseModeTransact = 1;

var msiViewModifyInsert         = 1
var msiViewModifyUpdate         = 2
var msiViewModifyAssign         = 3
var msiViewModifyReplace        = 4
var msiViewModifyDelete         = 6

var msidbCustomActionTypeInScript       = 0x00000400;
var msidbCustomActionTypeNoImpersonate  = 0x00000800

if (WScript.Arguments.Length != 1)
{
	WScript.StdErr.WriteLine(WScript.ScriptName + " file");
	WScript.Quit(1);
}

var filespec = WScript.Arguments(0);
WScript.Echo("Filespec: " + filespec);
var installer = WScript.CreateObject("WindowsInstaller.Installer");
var database = installer.OpenDatabase(filespec, msiOpenDatabaseModeTransact);

var sql
var view
var record

try
{
	sql = "SELECT `Action`, `Type`, `Source`, `Target` FROM `CustomAction`";
	view = database.OpenView(sql);
	view.Execute();
	record = view.Fetch();
	while (record)
	{
	    if (record.IntegerData(2) & msidbCustomActionTypeInScript)
	    {
	        record.IntegerData(2) = record.IntegerData(2) | msidbCustomActionTypeNoImpersonate;
        	view.Modify(msiViewModifyReplace, record);
	    }
        record = view.Fetch();
	}

	view.Close();
	database.Commit();
}
catch(e)
{
	WScript.StdErr.WriteLine(e);
	WScript.Quit(1);
}
