#include <Main.h>
#include <Utility.h>
#include <d3d9types.h>
#include <Core.h>
#include <winuser.h>
#include "DDSTextureLoader.h"
#include <fstream>
#include <sstream>
#include <TGA.h>
#include <shellapi.h>

namespace GW2Radial
{

std::string utf8_encode(const std::wstring &wstr)
{
    if( wstr.empty() ) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring utf8_decode(const std::string &str)
{
    if( str.empty() ) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void SplitFilename(const tstring& str, tstring* folder, tstring* file)
{
	const auto found = str.find_last_of(TEXT("/\\"));
	if (folder) *folder = str.substr(0, found);
	if (file) *file = str.substr(found + 1);
}

mstime TimeInMilliseconds()
{
	mstime iCount;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&iCount));
	mstime iFreq;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&iFreq));
	return 1000 * iCount / iFreq;
}

bool ShaderIsEnd(DWORD token)
{
	return (token & D3DSI_OPCODE_MASK) == D3DSIO_END;
}

int GetShaderFuncLength(const DWORD *pFunction)
{
	int op = 0, l = 1;
	while (!ShaderIsEnd(pFunction[op++]))  l++;
	return l;
}

std::span<byte> LoadResource(UINT resId)
{
    cref dll = Core::i().dllModule();
    const auto res = FindResource(dll, MAKEINTRESOURCE(resId), RT_RCDATA);
	if(res)
	{
        const auto handle = LoadResource(dll, res);
		if(handle)
		{
			size_t sz = SizeofResource(dll, res);
			void* ptr = LockResource(handle);

			return std::span<byte>((byte*)ptr, sz);
		}
	}

    return {};
}

std::pair<ComPtr<ID3D11Resource>, ComPtr<ID3D11ShaderResourceView>> CreateResourceFromResource(ID3D11Device* pDev, HMODULE hModule, unsigned uResource)
{
    const auto resourceSpan = LoadResource(uResource);
	if(resourceSpan.data() == nullptr)
		return { nullptr, nullptr };

	ComPtr<ID3D11Resource> res;
    ComPtr<ID3D11ShaderResourceView> srv;

    auto hr = DirectX::CreateDDSTextureFromMemory(pDev, resourceSpan.data(), resourceSpan.size_bytes(), &res, &srv);
    GW2_ASSERT(SUCCEEDED(hr));

	return { res, srv };
}

uint RoundUpToMultipleOf(uint numToRound, uint multiple)
{
    if (multiple == 0)
        return numToRound;

    uint remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

std::filesystem::path GetGameFolder()
{
    wchar_t exeFullPath[MAX_PATH];
    GetModuleFileNameW(nullptr, exeFullPath, MAX_PATH);
    std::wstring exeFolder;
    SplitFilename(exeFullPath, &exeFolder, nullptr);

    return exeFolder;
}

std::optional<std::filesystem::path> GetDocumentsFolder()
{
    wchar_t* myDocuments;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &myDocuments)))
        return std::nullopt;

    std::filesystem::path documentsGW2 = myDocuments;
    documentsGW2 /= L"GUILD WARS 2";

    if (std::filesystem::is_directory(documentsGW2))
        return documentsGW2;

    return std::nullopt;
}

std::optional<std::filesystem::path> GetAddonFolder()
{
    auto folder = (GetGameFolder() / "addons/gw2radial").make_preferred();

    if (std::filesystem::is_directory(folder))
        return folder;

    if (SUCCEEDED(SHCreateDirectoryExW(nullptr, folder.c_str(), nullptr)))
        return folder;

    Log::i().Print(Severity::Warn, L"Could not open or create configuration folder '{}'.", folder.wstring());

    auto docs = GetDocumentsFolder();
    if (!docs) {
        Log::i().Print(Severity::Error, L"Could not locate Documents folder (fallback).");
        return std::nullopt;
    }

    folder = (*docs / "addons/gw2radial").make_preferred();

    if (std::filesystem::is_directory(folder))
        return folder;

    if (SUCCEEDED(SHCreateDirectoryExW(nullptr, folder.c_str(), nullptr)))
        return folder;

    Log::i().Print(Severity::Error, L"Could not open or create configuration folder '{}'.", folder.wstring());

    return std::nullopt;
}

std::span<const wchar_t*> GetCommandLineArgs() {
    auto cmdLine = GetCommandLineW();
    int num = 0;
    wchar_t** args = CommandLineToArgvW(cmdLine, &num);

    return std::span { const_cast<const wchar_t**>(args), size_t(num) };
}

const wchar_t* GetCommandLineArg(const wchar_t* name) {
    bool saveNextArg = false;
    for (auto* arg : GetCommandLineArgs()) {
        if (saveNextArg) {
            return arg;
        }

        auto l = wcslen(arg);
        if (l > 1 && (arg[0] == L'/' || arg[0] == L'-') && _wcsnicmp(name, &arg[1], 6) == 0) {
            if (l > 7 && arg[7] == L':') {
                return &arg[8];
                break;
            }
            else
                saveNextArg = true;
        }
    }

    return nullptr;
}

void DrawScreenQuad(ComPtr<ID3D11DeviceContext>& ctx)
{
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->Draw(4, 0);
}

}
