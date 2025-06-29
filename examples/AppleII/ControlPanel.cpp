#include "ControlPanel.h"
#include "Computer.h"
#include "roms/Roms.h"
#include <Dsk2Nib.h>

#include <portable-file-dialogs.h>
#include <Fs.h>

namespace qe::Examples::AppleII
{

void ControlPanel::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool ControlPanel::CreateModule(int &width, int &height, std::string &title)
{
    width = 800;
    height = 600;
    title = "AppleII Control Panel";
    LoadUserspace();

    return true;
}

void ControlPanel::RenderModule()
{
    ImGui::Begin("AppleII Control Panel");

    ShowOpenFile();
    ShowCpuCtrl();

    ImGui::End();
}

void ControlPanel::CloseModule()
{

}

void ControlPanel::ShowOpenFile()
{
    if (ImGui::Button("Diskette..."))
    {
        std::string folder = userspace_.value("last_folder", ".");
        std::vector<std::string> file = pfd::open_file("Open Floppy Disk Image", folder, { "Image Files", "*.dsk *.nib" }).result();
        if (!file.empty() && !file.at(0).empty())
        {
            diskImageFile_ = file.at(0);
            userspace_["last_folder"] = diskImageFile_.parent_path().string();
            userspace_["last_file"] = diskImageFile_;
            SaveUserspace();
        }
    }
    ImGui::Text("Diskette: %s", diskImageFile_.filename().c_str());
}

void ControlPanel::ShowCpuCtrl()
{
    ShowStartControl();
    if (ctx_.computer->IsOn())
    {
        ShowStopControl();
        ShowSpeedControl();
        ShowPauseControl();
    }
}

void ControlPanel::ShowStartControl()
{
    const char* txt = ctx_.computer->IsOn() ? "Restart" : "Start";
    if (ImGui::Button(txt))
    {
        OnRun();
    }
}

void ControlPanel::ShowStopControl()
{
    if (ImGui::Button("Stop"))
    {
        ctx_.computer->Shutdown();
    }
}

void ControlPanel::ShowSpeedControl()
{
    if (ctx_.computer->IsTurboSpeed())
    {
        ImGui::SameLine();
        if (ImGui::Button("Normal"))
        {
            ctx_.computer->NormalSpeed();
        }
    }
    else
    {
        ImGui::SameLine();
        if (ImGui::Button("Turbo"))
        {
            ctx_.computer->TurboSpeed();
        }
    }
}

void ControlPanel::ShowPauseControl()
{
    if (ctx_.computer->IsRunning())
    {
        ImGui::SameLine();
        if (ImGui::Button("Pause"))
        {
            ctx_.computer->Pause();
        }
    }
    else
    {
        ImGui::SameLine();
        if (ImGui::Button("Resume"))
        {
            ctx_.computer->Resume();
        }
    }
}

void ControlPanel::SaveUserspace()
{
    Fs::SaveTextFile( "./AppleII_Userspace.json", userspace_.dump(3) );
}

void ControlPanel::LoadUserspace()
{
    auto file = Fs::LoadTextFile("./AppleII_Userspace.json");
    if (!file.empty())
    {
        userspace_ = Json::parse( file );
        diskImageFile_ = userspace_.value("last_file", "");
    }
    else
    {
        userspace_["last_folder"] = ".";
    }
}

std::vector<uint8_t> ControlPanel::LoadDiskette(Fs::Path file)
{
    std::vector<uint8_t> diskette;
    diskette.clear();
    if (!file.empty())
    {
        std::filesystem::path p(file);
        std::string ext = p.extension().string();
        if (ext == ".dsk")
        {
            diskette = Fs::Dsk2Nib(file.c_str());
        }
        else if (ext == ".nib")
        {
            diskette = Fs::LoadBinaryFile(file);
        }
        else
        {
            fmt::println("Unknown file format: {}", ext);
        }
    }
    return diskette;
}

void ControlPanel::OnRun()
{
    auto diskette = LoadDiskette(diskImageFile_);
    ctx_.computer->Boot(Roms::Get_AppleII_Plus_DiskII_Card5(),
                        Roms::Get_AppleII_Plus_VideoRom(),
                        diskette);
}

}
