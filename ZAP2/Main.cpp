#include "ZFile.h"
#include "ZTexture.h"
#include "ZBlob.h"
#include "ZAnimation.h"
#include "HighLevel/HLModelIntermediette.h"
#include "HighLevel/HLAnimationIntermediette.h"
#include "Overlays/ZOverlay.h"
#include "Path.h"
#include "File.h"
#include "Globals.h"

#ifndef _MSC_VER
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

#include <string>
#include "tinyxml2.h"

extern void ModelTest();
extern void ModelTest2();

using namespace tinyxml2;
using namespace std;

bool Parse(string xmlFilePath, string basePath, string outPath, ZFileMode fileMode);

void BuildAssetTexture(string pngFilePath, TextureType texType, string outPath);
void BuildAssetBlob(string blobFilePath, string outPath);
void BuildAssetModelIntermediette(string mdlPath, string outPath);
void BuildAssetAnimationIntermediette(string animPath, string outPath);

int NewMain(int argc, char* argv[]);

#ifndef _MSC_VER
void ErrorHandler(int sig)
{
	void* array[4096];
	char** symbols;
	size_t size;
	size = backtrace(array, 4096);
	symbols = backtrace_symbols(array, 4096);

	for (int i = 1; i < size; i++)
	{
		size_t len = strlen(symbols[i]);
		cout << symbols[i] << "\n";
	}

	//cout << "Error: signal " << sig << ":\n";
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}
#endif

int main(int argc, char* argv[])
{
#ifndef _MSC_VER
	//signal(SIGSEGV, ErrorHandler);
#endif

	Globals* g = new Globals();

	return NewMain(argc, argv);
}

int NewMain(int argc, char* argv[])
{
	// Syntax: ZAP2.exe [mode (b/btex/bovl/e)] (Arbritrary Number of Arguments)
	printf("ZAPD: Zelda Asset Processor For Decomp\n");

	// Parse File Mode
	string buildMode = argv[1];
	ZFileMode fileMode = ZFileMode::Invalid;

	if (buildMode == "b")
		fileMode = ZFileMode::Build;
	else if (buildMode == "btex")
		fileMode = ZFileMode::BuildTexture;
	else if (buildMode == "bovl")
		fileMode = ZFileMode::BuildOverlay;
	else if (buildMode == "bsf")
		fileMode = ZFileMode::BuildSourceFile;
	else if (buildMode == "bblb")
		fileMode = ZFileMode::BuildBlob;
	else if (buildMode == "bmdlintr")
		fileMode = ZFileMode::BuildModelIntermediette;
	else if (buildMode == "bamnintr")
		fileMode = ZFileMode::BuildAnimationIntermediette;
	else if (buildMode == "e")
		fileMode = ZFileMode::Extract;

	if (fileMode == ZFileMode::Invalid)
	{
		printf("Error: Invalid file mode '%s'\n", buildMode.c_str());
		return 1;
	}

	// Parse other "commands"
	for (int i = 2; i < argc; i++)
	{
		string arg = argv[i];

		if (arg == "-o" || arg == "--outputpath") // Set output path
		{
			Globals::Instance->outputPath = argv[i + 1];
			i++;
		}
		else if (arg == "-i" || arg == "--inputpath") // Set input path
		{
			Globals::Instance->inputPath = argv[i + 1];
			i++;
		}
		else if (arg == "-b" || arg == "--baserompath") // Set baserom path
		{
			Globals::Instance->baseRomPath = argv[i + 1];
			i++;
		}
		else if (arg == "-gsf") // Generate source file during extraction
		{
			Globals::Instance->genSourceFile = string(argv[i + 1]) == "1";
			i++;
		}
		else if (arg == "-ifp") // Include file prefix in generated symbols
		{
			Globals::Instance->includeFilePrefix = string(argv[i + 1]) == "1";
			i++;
		}
		else if (arg == "-tm") // Test Mode
		{
			Globals::Instance->testMode = string(argv[i + 1]) == "1";
			i++;
		}
		else if (arg == "-profile") // Profile
		{
			Globals::Instance->profile = string(argv[i + 1]) == "1";
			i++;
		}
		else if (arg == "-uer") // Split resources into their individual components (enabled by default)
		{
			Globals::Instance->useExternalResources = string(argv[i + 1]) == "1";
			i++;
		}
		else if (arg == "-tt") // Set texture type
		{
			Globals::Instance->texType = ZTexture::GetTextureTypeFromString(argv[i + 1]);
			i++;
		}
		else if (arg == "-cfg") // Set cfg path
		{
			Globals::Instance->cfgPath = argv[i + 1];
			i++;
		}
		else if (arg == "-sm") // Set symbol map path
		{
			Globals::Instance->GenSymbolMap(argv[i + 1]);
			i++;
		}
		else if (arg == "-al") // Set actor list
		{
			i++;
		}
		else if (arg == "-ol") // Set object list
		{
			i++;
		}
		else if (arg == "-eh") // Enable Error Handler
		{
#ifndef _MSC_VER
			signal(SIGSEGV, ErrorHandler);
#endif
		}
	}

	if (fileMode == ZFileMode::Build || fileMode == ZFileMode::Extract || fileMode == ZFileMode::BuildSourceFile)
	{
		Parse(Globals::Instance->inputPath, Globals::Instance->baseRomPath, Globals::Instance->outputPath, fileMode);
	}
	else if (fileMode == ZFileMode::BuildTexture)
	{
		TextureType texType = Globals::Instance->texType;
		string pngFilePath = Globals::Instance->inputPath;
		string outFilePath = Globals::Instance->outputPath;

		BuildAssetTexture(pngFilePath, texType, outFilePath);
	}
	else if (fileMode == ZFileMode::BuildBlob)
	{
		string blobFilePath = Globals::Instance->inputPath;
		string outFilePath = Globals::Instance->outputPath;

		BuildAssetBlob(blobFilePath, outFilePath);
	}
	else if (fileMode == ZFileMode::BuildModelIntermediette)
	{
		BuildAssetModelIntermediette(Globals::Instance->inputPath, Globals::Instance->outputPath);
	}
	else if (fileMode == ZFileMode::BuildAnimationIntermediette)
	{
		BuildAssetAnimationIntermediette(Globals::Instance->inputPath, Globals::Instance->outputPath);
	}
	else if (fileMode == ZFileMode::BuildOverlay)
	{
		ZOverlay* overlay = ZOverlay::FromBuild(Path::GetDirectoryName(Globals::Instance->inputPath), Path::GetDirectoryName(Globals::Instance->cfgPath));
		if (overlay)
		{
			string source = overlay->GetSourceOutputCode("");
			File::WriteAllText(Globals::Instance->outputPath, source);
		}
	}

	return 0;
}

bool Parse(string xmlFilePath, string basePath, string outPath, ZFileMode fileMode)
{
	XMLDocument doc;
	XMLError eResult = doc.LoadFile(xmlFilePath.c_str());

	if (eResult != tinyxml2::XML_SUCCESS)
		return false;

	XMLNode* root = doc.FirstChild();

	if (root == nullptr)
		return false;

	vector<ZFile*> files = vector<ZFile*>();

	for (XMLElement* child = root->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		if (string(child->Name()) == "File")
		{
			ZFile* file = new ZFile(fileMode, child, basePath, outPath);
			files.push_back(file);
		}
	}

	for (ZFile* file : files)
	{
		if (fileMode == ZFileMode::Build)
			file->BuildResources();
		else if (fileMode == ZFileMode::BuildSourceFile)
			file->BuildSourceFile(outPath);
		else
			file->ExtractResources(outPath);
	}

	XMLElement* element = root->FirstChildElement("File");

	if (element == nullptr)
		return false;

	return true;
}

void BuildAssetTexture(string pngFilePath, TextureType texType, string outPath)
{
	vector<string> split = StringHelper::Split(outPath, "/");
	string name = StringHelper::Split(split[split.size() - 1], ".")[0];
	ZTexture* tex = ZTexture::FromPNG(pngFilePath, texType);

	string src = StringHelper::Sprintf("u64 %s[] = \n{\n", name.c_str()) + tex->GetSourceOutputCode(name) + "};\n";

	File::WriteAllText(outPath, src);
	
	delete tex;
}

void BuildAssetBlob(string blobFilePath, string outPath)
{
	vector<string> split = StringHelper::Split(outPath, "/");
	ZBlob* blob = ZBlob::FromFile(blobFilePath);
	string name = StringHelper::Split(split[split.size() - 1], ".")[0];

	string src = StringHelper::Sprintf("u8 %s[] = \n{\n", name.c_str()) + blob->GetSourceOutputCode(name) + "};\n";

	File::WriteAllText(outPath, src);

	delete blob;
}

void BuildAssetModelIntermediette(string mdlPath, string outPath)
{
	XMLDocument doc;
	XMLError eResult = doc.LoadFile(mdlPath.c_str());

	vector<string> split = StringHelper::Split(outPath, "/");
	HLModelIntermediette* mdl = HLModelIntermediette::FromXML(doc.RootElement());
	
	string output = "";

	output += mdl->OutputCode();
	
	File::WriteAllText(outPath, output);

	delete mdl;
}

void BuildAssetAnimationIntermediette(string animPath, string outPath)
{
	vector<string> split = StringHelper::Split(outPath, "/");
	ZFile* file = new ZFile("", split[split.size() - 2]);
	HLAnimationIntermediette* anim = HLAnimationIntermediette::FromXML(animPath);
	ZAnimation* zAnim = anim->ToZAnimation();
	zAnim->SetName(Path::GetFileNameWithoutExtension(split[split.size() - 1]));
	zAnim->parent = file;
	zAnim->rotationIndicesSeg = 1;
	zAnim->rotationValuesSeg = 2;

	zAnim->GetSourceOutputCode(split[split.size() - 2]);
	string output = "";
	
	output += file->declarations[2]->text + "\n";
	output += file->declarations[1]->text + "\n";
	output += file->declarations[0]->text + "\n";

	File::WriteAllText(outPath, output);

	delete zAnim;
	delete file;
}