#include "clConsoleCodeLiteTerminal.h"
#include "cl_standard_paths.h"
#include "fileutils.h"

clConsoleCodeLiteTerminal::clConsoleCodeLiteTerminal()
{
    wxString cmd = GetBinary();
    WrapWithQuotesIfNeeded(cmd);
    SetTerminalCommand(wxString() << cmd << " --working-directory=%WD% --file=%COMMANDFILE%");
    SetEmptyTerminalCommand(wxString() << cmd << " --working-directory=%WD%");
}

clConsoleCodeLiteTerminal::~clConsoleCodeLiteTerminal() {}

wxString clConsoleCodeLiteTerminal::PrepareCommand()
{
    // Build the command to execute
    wxString commandToExecute;
    if(IsTerminalNeeded()) {
        wxFileName tmpfile(clStandardPaths::Get().GetTempDir(), "codelite-terminal.txt");
        bool hasCommand = !GetCommand().IsEmpty();
        commandToExecute = hasCommand ? GetTerminalCommand() : GetEmptyTerminalCommand();

        // For testing purposes
        wxString command = WrapWithQuotesIfNeeded(GetCommand());
        if(!command.IsEmpty()) {
#ifdef __WXMSW__
            command.Prepend("start /B /WAIT "); // start the application in the foreground
#endif
            if(!GetCommandArgs().IsEmpty()) {
                wxString cmdArgs = GetCommandArgs();
                command << " " << GetCommandArgs();
            }

            // Write the content of the command into a file
            FileUtils::WriteFileContent(tmpfile, command);
        }

        wxString wd = GetWorkingDirectory();
        if(wd.IsEmpty() || !wxFileName::DirExists(wd)) { wd = WrapWithQuotesIfNeeded(wxGetCwd()); }
        if(IsWaitWhenDone()) { commandToExecute << " --wait "; }

        wxString commandFile = tmpfile.GetFullPath();
        commandFile = WrapWithQuotesIfNeeded(commandFile);

        commandToExecute.Replace("%COMMANDFILE%", commandFile);
        commandToExecute.Replace("%WD%", wd);

    } else {
        commandToExecute = WrapWithQuotesIfNeeded(GetCommand());
        if(!GetCommandArgs().IsEmpty()) { commandToExecute << " " << GetCommandArgs(); }
    }
    return commandToExecute;
}

bool clConsoleCodeLiteTerminal::StartForDebugger()
{
#ifdef __WXMSW__
    return false;
#else
    // generate a random value to differntiate this instance of codelite
    // from other instances

    time_t curtime = time(NULL);
    int randomSeed = (curtime % 947);
    wxString secondsToSleep;

    secondsToSleep << (85765 + randomSeed);

    wxString sleepCommand = "/bin/sleep";
    sleepCommand << " " << secondsToSleep;

    wxString homedir = wxGetHomeDir();
    if(homedir.Contains(" ")) { homedir.Prepend("\"").Append("\""); }
    wxString commandToExecute;
    commandToExecute << GetBinary();
    WrapWithQuotesIfNeeded(commandToExecute);

    commandToExecute << " --working-directory=" << homedir << " --command=\"" << sleepCommand << "\"";
    ::wxExecute(commandToExecute);

    // Let it start ... (wait for it up to 5 seconds)
    for(size_t i = 0; i < 100; ++i) {
        if(FindProcessByCommand(sleepCommand, m_tty, m_pid)) {
            // On GTK, redirection to TTY does not work with lldb
            // as a workaround, we create a symlink with different name

            // Keep the real tty
            m_realPts = m_tty;

            wxString symlinkName = m_tty;
            symlinkName.Replace("/dev/pts/", "/tmp/pts");
            wxString lnCommand;
            lnCommand << "ln -sf " << m_tty << " " << symlinkName;
            if(::system(lnCommand.mb_str(wxConvUTF8).data()) == 0) { m_tty.swap(symlinkName); }
            break;
        }
        wxThread::Sleep(50);
    }
    return !m_tty.IsEmpty();
#endif
}

wxString clConsoleCodeLiteTerminal::GetBinary() const
{
    wxFileName codeliteTerminal(clStandardPaths::Get().GetBinFolder(), "codelite-terminal");
#ifdef __WXMSW__
    codeliteTerminal.SetExt("exe");
#endif
    return codeliteTerminal.GetFullPath();
}
