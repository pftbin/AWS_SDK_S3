#include "stdafx.h"
#include "C_AWSServer.h"

DECLARE_LOGER(LogerDVInterface);

#define DF_RETRY_TIMES  3

//AWS
#pragma comment(lib, "aws-c-auth.lib") 
#pragma comment(lib, "aws-c-cal.lib") 
#pragma comment(lib, "aws-c-common.lib") 
#pragma comment(lib, "aws-c-compression.lib") 
#pragma comment(lib, "aws-c-event-stream.lib") 
#pragma comment(lib, "aws-checksums.lib") 
#pragma comment(lib, "aws-c-http.lib") 
#pragma comment(lib, "aws-c-io.lib") 
#pragma comment(lib, "aws-c-mqtt.lib") 
#pragma comment(lib, "aws-crt-cpp.lib") 
#pragma comment(lib, "aws-c-s3.lib") 
#pragma comment(lib, "aws-c-sdkutils.lib") 
#pragma comment(lib, "aws-cpp-sdk-core.lib") 
#pragma comment(lib, "aws-cpp-sdk-s3.lib") 

static void unicode_to_utf8(const wchar_t* in, size_t len, std::string& out)
{
#if defined WIN32
	int out_len = ::WideCharToMultiByte(CP_UTF8, 0, in, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
	if (out_len > 0) {
		char* lpBuf = new char[static_cast<unsigned int>(out_len + 1)];
		if (lpBuf) {
			memset(lpBuf, 0, static_cast<unsigned int>(out_len + 1));
			int return_len = ::WideCharToMultiByte(CP_UTF8, 0, in, static_cast<int>(len), lpBuf, out_len, nullptr, nullptr);
			if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
			delete[]lpBuf;
		}
	}
#else
	/*if (wcslen(in) <= 0 || len <= 0) return;
	std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
	size_t w_len = len * 4 + 1;
	setlocale(LC_CTYPE, "en_US.UTF-8");
	std::unique_ptr<char[]> p(new char[w_len]);
	size_t return_len = wcstombs(p.get(), in, w_len);
	if (return_len > 0) out.assign(p.get(), return_len);
	setlocale(LC_CTYPE, "C");*/

	size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
	char* save = new char[w_len];
	memset(save, '\0', w_len);
	iconv_convert("UTF-32", "UTF-8//IGNORE", save, w_len, (char*)(in), w_len);
	out = save;
	delete[]save;
#endif
}

static void utf8_to_unicode(const char* in, size_t len, std::wstring& out)
{
#if defined WIN32
	int out_len = ::MultiByteToWideChar(CP_UTF8, 0, in, static_cast<int>(len), nullptr, 0);
	if (out_len > 0) {
		wchar_t* lpBuf = new wchar_t[static_cast<unsigned int>(out_len + 1)];
		if (lpBuf) {
			memset(lpBuf, 0, (static_cast<unsigned int>(out_len + 1)) * sizeof(wchar_t));
			int return_len = ::MultiByteToWideChar(CP_UTF8, 0, in, static_cast<int>(len), lpBuf, out_len);
			if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
			delete[]lpBuf;
		}
	}
#else
	/*if (strlen(in) <= 0 || len <= 0) return;
	std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
	setlocale(LC_CTYPE, "en_US.UTF-8");
	std::unique_ptr<wchar_t[]> p(new wchar_t[sizeof(wchar_t) * (len + 1)]);
	size_t return_len = mbstowcs(p.get(), in, len + 1);
	if (return_len > 0) out.assign(p.get(), return_len);
	setlocale(LC_CTYPE, "C");*/

	size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
	char* save = new char[w_len];
	memset(save, '\0', w_len);
	iconv_convert("UTF-8", "UTF-32//IGNORE", save, w_len, (char*)(in), len);
	out = (wchar_t*)save + 1;
	delete[]save;
#endif
}

static void ansi_to_unicode(const char* in, size_t len, std::wstring& out)
{
#if defined WIN32
	int out_len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in, static_cast<int>(len), nullptr, 0);
	if (out_len > 0) {
		wchar_t* lpBuf = new wchar_t[static_cast<unsigned int>(out_len + 1)];
		if (lpBuf) {
			memset(lpBuf, 0, (static_cast<unsigned int>(out_len + 1)) * sizeof(wchar_t));
			int return_len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in, static_cast<int>(len), lpBuf, out_len);
			if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
			delete[]lpBuf;
		}
	}
#else
	/*mbstate_t state;
	memset (&state, '\0', sizeof (state));
	std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
	//setlocale(LC_CTYPE, "zh_CN.UTF-8");
	setlocale(LC_CTYPE, "en_US.UTF-8");
	size_t out_len= mbsrtowcs(nullptr, &in, 0, &state);
	if (out_len > 0 &&  out_len < UINT_MAX && len > 0)
	{
		std::unique_ptr<wchar_t[]> lpBuf(new wchar_t[sizeof(wchar_t) * (out_len + 1)]);
		size_t return_len = mbsrtowcs(lpBuf.get(), &in, out_len+1, &state);
		if (return_len > 0) out.assign(lpBuf.get(), return_len);
	}
	setlocale(LC_CTYPE, "C");*/

	size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
	char* save = new char[w_len];
	memset(save, '\0', w_len);

	iconv_convert("GBK", "UTF-32//IGNORE", save, w_len, (char*)(in), len);
	out = (wchar_t*)save + 1;
	delete[]save;
#endif
}

static void unicode_to_ansi(const wchar_t* in, size_t len, std::string& out)
{
#if defined WIN32
	int out_len = ::WideCharToMultiByte(CP_ACP, 0, in, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
	if (out_len > 0) {
		char* lpBuf = new char[static_cast<unsigned int>(out_len + 1)];
		if (lpBuf) {
			memset(lpBuf, 0, static_cast<unsigned int>(out_len + 1));
			int return_len = ::WideCharToMultiByte(CP_ACP, 0, in, static_cast<int>(len), lpBuf, out_len, nullptr, nullptr);
			if (return_len > 0) out.assign(lpBuf, static_cast<unsigned int>(return_len));
			delete[]lpBuf;
		}
	}
#else
	/*mbstate_t state;
	memset(&state, '\0', sizeof(state));
	std::lock_guard<std::mutex> guard(g_ConvertCodeMutex);
	setlocale(LC_CTYPE, "en_US.UTF-8");
	size_t out_len = wcsrtombs(nullptr, &in, 0, &state);
	if (out_len > 0 && out_len < UINT_MAX && len > 0)
	{
		std::unique_ptr<char[]> lpBuf(new char[sizeof(char) *(out_len + 1)]);
		size_t return_len = wcsrtombs(lpBuf.get(), &in, out_len+1, &state);//wcstombs(lpBuf.get(), in, len + 1);
		if (return_len > 0) out.assign(lpBuf.get(), return_len);
	}
	setlocale(LC_CTYPE, "C");*/

	size_t w_len = len * (sizeof(wchar_t) / sizeof(char)) + 1U;
	char* save = new char[w_len];
	memset(save, '\0', w_len);
	iconv_convert("UTF-32", "GBK//IGNORE", save, w_len, (char*)(in), w_len - 1U);
	out = save;
	delete[]save;
#endif
}

static void ansi_to_utf8(const char* in, size_t len, std::string& out)
{
#if defined WIN32
	std::wstring strUnicode;
	ansi_to_unicode(in, len, strUnicode);
	return unicode_to_utf8(strUnicode.c_str(), strUnicode.length(), out);
#else
	/*std::wstring strUnicode;
	ansi_to_unicode(in, len, strUnicode);
	return unicode_to_utf8(strUnicode.c_str(), strUnicode.length(), out);*/

	size_t w_len = len * (sizeof(wchar_t) / sizeof(char));
	char* save = new char[w_len];
	memset(save, '\0', w_len);
	iconv_convert("GBK", "UTF-8//IGNORE", save, w_len, (char*)(in), len);
	out = save;
	delete[]save;
#endif
}

static void utf8_to_ansi(const char* in, size_t len, std::string& out)
{
#if defined WIN32
	std::wstring strUnicode;
	utf8_to_unicode(in, len, strUnicode);
	return unicode_to_ansi(strUnicode.c_str(), strUnicode.length(), out);
#else
	/*std::wstring strUnicode;
	utf8_to_unicode(in, len, strUnicode);
	return unicode_to_ansi(strUnicode.c_str(), strUnicode.length(), out);*/

	size_t w_len = len * (sizeof(wchar_t) / sizeof(char));
	char* save = new char[w_len];
	memset(save, '\0', w_len);
	iconv_convert("UTF-8", "GBK//IGNORE", save, w_len, (char*)(in), len);
	out = save;
	delete[]save;
#endif
}

//判断文件是否存在
inline bool file_exists(const string& name)
{
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

//字符串替换
std::string str_replace(std::string str, std::string old, std::string now)
{
	if (str.empty())
		return "";

	int oldPos = 0;
	while (str.find(old, oldPos) != -1)
	{
		int start = str.find(old, oldPos);
		str.replace(start, old.size(), now);
		oldPos = start + now.size();
	}
	return str;
}

//获得文件大小，单位字节
inline size_t get_filesize(const char* fileName) 
{
	if (fileName == nullptr) 
		return 0;

	// 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
	struct stat statbuf;

	// 提供文件名字符串，获得文件属性结构体
	stat(fileName, &statbuf);

	// 获取文件大小
	size_t filesize = statbuf.st_size;

	return filesize;
}

//从路径获取文件夹
std::string get_path_folder(std::string filepath)
{
	std::string folder = "";
	//从最后一个反斜杠或正斜杠处分离目录名
	size_t pos = filepath.find_last_of("/\\");
	if (pos != std::string::npos) {
		folder = filepath.substr(0, pos);
	}
	return folder;
}

//从路径获取文件名
std::string get_path_name(std::string filepath)
{
	std::string name = "";
	//从最后一个反斜杠或正斜杠处分离文件名
	size_t pos = filepath.find_last_of("\\/");
	if (pos != std::string::npos)
		name = filepath.substr(pos + 1);

	return name;
}

//获取文件夹所有文件
static void GetPathAllFile(CString strDir, std::vector<CString>& vFilePathList)
{
	CFileFind finder;
	BOOL isNotEmpty = finder.FindFile(strDir + _T("*.*"));//总文件夹，开始遍历 
	while (isNotEmpty)
	{
		isNotEmpty = finder.FindNextFile();//查找文件 
		CString filename = finder.GetFilePath();//获取文件的路径，可能是文件夹，可能是文件 
		if (!(finder.IsDirectory()))
		{
			//如果是文件则加入文件列表 
			vFilePathList.push_back(filename);//将一个文件路径加入容器 
		}
		else
		{
			//递归遍历用户文件夹，跳过非用户文件夹 
			if (!(finder.IsDots() || finder.IsHidden() || finder.IsSystem() || finder.IsTemporary() || finder.IsReadOnly()))
			{
				GetPathAllFile(filename + _T("\\"), vFilePathList);
			}
		}
	}
}
//获取文件名
static CString NameFromPath(CString& strFullPath, BOOL bExtension = TRUE)
{
	CString strFinalName;
	if (strFullPath.IsEmpty())
		return strFinalName;

	strFullPath.Replace(_T("\\"), _T("/"));
	CString strFullName = strFullPath.Right(strFullPath.GetLength() - strFullPath.ReverseFind('/') - 1); //带后缀的文件名
	CString strFirstName = strFullName.Left(strFullName.Find('.')); //去除后缀
	if (bExtension)
		strFinalName = strFullName;
	else
		strFinalName = strFirstName;

	return strFinalName;
}
//获取文件夹
static CString FolderFromPath(CString& strFullPath)
{
	CString strFolder = _T("");
	int nPos = strFullPath.ReverseFind('/');
	if (nPos == -1)
		return strFolder;
	strFolder = strFullPath.Left(nPos);//获取文件夹

	return strFolder;
}

C_AWSServer::C_AWSServer(void)
{
	m_strEndPoint = _T("");
	m_strAK = _T("");
	m_strSK = _T("");
	m_strBucket = _T("");
	m_strRegion = _T("");
	m_bOssConfig = FALSE;
	Aws::InitAPI(m_aws_options);
}

C_AWSServer::~C_AWSServer(void)
{
	Aws::ShutdownAPI(m_aws_options);
}

////////////////////////////////////////////////////////////////////////////////////////
BOOL C_AWSServer::IsEnable()
{
	return m_bOssConfig;
}

BOOL C_AWSServer::SetAWSConfig(CString strWebPrefix, CString strEndPoint, CString strUrl, CString strAK, CString strSK, CString strBucket, CString strRegion)
{
	if (strEndPoint.IsEmpty() || strUrl.IsEmpty() || strAK.IsEmpty() || strSK.IsEmpty() || strBucket.IsEmpty())
	{
		m_bOssConfig = FALSE;
		return FALSE;
	}

	m_strWebPrefix = strWebPrefix;
	m_strEndPoint = strEndPoint;
	m_strUrl = strUrl;
	m_strAK  = strAK;
	m_strSK  = strSK;
	m_strBucket = strBucket;
	m_strRegion = strRegion;
	m_bOssConfig = TRUE;
	return TRUE;
}

BOOL C_AWSServer::UploadFile(CString strObjectFolder, CString strLocalFilePath, CString& strUrlFilePath)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[UploadFile] OSS路径[ %s ],本地文件[ %s ]"), strObjectFolder.GetBuffer(), strLocalFilePath.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	
	if (strLocalFilePath.IsEmpty())
		return FALSE;
	strObjectFolder.Replace(_T("\\"), _T("/"));
	strLocalFilePath.Replace(_T("\\"), _T("/"));

	//Object路径
	CString strFullName = NameFromPath(strLocalFilePath,TRUE);
	if (strObjectFolder.Right(1) != _T("/"))
		strObjectFolder += _T("/");
	CString strObjectFilePath = strObjectFolder + strFullName;

	//上传
	BOOL bResult = FALSE;
	std::string s_objectfile_path = (CT2A)strObjectFilePath;
	std::string s_localfile_path = (CT2A)strLocalFilePath;
	std::string s_urlfile_path;
	for (int i = 0; i < DF_RETRY_TIMES; i++)
	{
		ansi_to_utf8(s_objectfile_path.c_str(), s_objectfile_path.length(), s_objectfile_path);
		ansi_to_utf8(s_localfile_path.c_str(), s_localfile_path.length(), s_localfile_path);
		if (put_s3_object_multipart(s_objectfile_path, s_localfile_path, s_urlfile_path))
		{
			bResult = TRUE;
			strUrlFilePath = s_urlfile_path.c_str();
			break;
		}
	}

	return bResult;
}

BOOL C_AWSServer::UploadFolder(CString strObjectFolder, CString strLocalFolder)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[UploadFolder] OSS路径[ %s ],本地文件夹[ %s ]"), strObjectFolder.GetBuffer(), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	if (strObjectFolder.IsEmpty() || strLocalFolder.IsEmpty())
		return FALSE;
	strObjectFolder.Replace(_T("\\"), _T("/"));
	strLocalFolder.Replace(_T("\\"), _T("/"));

	std::vector<CString> vecFilePath;
	GetPathAllFile(strLocalFolder, vecFilePath);
	if (vecFilePath.size() <= 0) return FALSE;

	BOOL bResult = TRUE;
	for (size_t i = 0; i < vecFilePath.size(); i++)
	{
		CString urlfile_path;
		CString localfile_path = vecFilePath[i]; localfile_path.Replace(_T("\\"), _T("/"));		//文件的路径，如 D:/AAA/BBB/123.mp4
		CString local_filefolder = FolderFromPath(localfile_path);								//文件夹路径，如 D:/AAA/BBB

		CString object_fullfolder = strObjectFolder;
		CString object_subfolder = local_filefolder; 
		object_subfolder.Replace(strLocalFolder, _T(""));										//在文件夹路径中删除根目录，如 D:/AAA
		if (object_subfolder.GetLength() > 1)													//最后长度大于1，则表名有子文件夹，如 /BBB
			object_fullfolder += object_subfolder;

		if (!UploadFile(object_fullfolder, localfile_path, urlfile_path))
			bResult &= FALSE;
	}

	return bResult;
}

BOOL C_AWSServer::DownloadFile(CString strObjectFilePath, CString strLocalFilePath)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[DownloadFile] OSS文件路径[ %s ],本地文件[ %s ]"), strObjectFilePath.GetBuffer(), strLocalFilePath.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	if (strObjectFilePath.IsEmpty() || strLocalFilePath.IsEmpty())
		return FALSE;
	strObjectFilePath.Replace(_T("\\"), _T("/"));
	strLocalFilePath.Replace(_T("\\"), _T("/"));

	CString strLocalFolder = FolderFromPath(strLocalFilePath);
	strLogInfo.Format(_T("[DownloadFile] 创建本地文件夹[%s]"), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (!::PathFileExists(strLocalFolder))
	{
		CString strNowLocalFolder = strLocalFolder;
		strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx不支持左斜杠
		::SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
		if (!::PathFileExists(strNowLocalFolder))
		{
			strLogInfo.Format(_T("[DownloadFile] 创建本地文件夹失败...[%s]"), strNowLocalFolder.GetBuffer());
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}
	}

	//下载
	BOOL bResult = FALSE;
	std::string s_objectfile_path = (CT2A)strObjectFilePath;
	std::string s_localfile_path = (CT2A)strLocalFilePath;
	for (int i = 0; i < DF_RETRY_TIMES; i++)
	{
		ansi_to_utf8(s_objectfile_path.c_str(), s_objectfile_path.length(), s_objectfile_path);
		ansi_to_utf8(s_localfile_path.c_str(), s_localfile_path.length(), s_localfile_path);
		if (get_s3_object_multipart(s_objectfile_path, s_localfile_path))
		{
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

BOOL C_AWSServer::DownloadFolder(CString strObjectFolder, CString strLocalFolder)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[DownloadFolder] OSS路径[ %s ],本地文件夹[ %s ]"), strObjectFolder.GetBuffer(), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (strObjectFolder.IsEmpty() || strLocalFolder.IsEmpty())
		return FALSE;
	strObjectFolder.Replace(_T("\\"), _T("/"));
	strLocalFolder.Replace(_T("\\"), _T("/"));

	strLogInfo.Format(_T("[DownloadFolder] 创建本地文件夹[%s]"), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (!::PathFileExists(strLocalFolder))
	{
		CString strNowLocalFolder = strLocalFolder; 
		strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx不支持左斜杠
		::SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
		if (!::PathFileExists(strNowLocalFolder))
		{
			strLogInfo.Format(_T("[DownloadFolder] 创建本地文件夹失败...[%s]"), strNowLocalFolder.GetBuffer());
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}
	}
	std::string object_folder = (CT2A)strObjectFolder;
	std::string local_folder = (CT2A)strLocalFolder;
	if (local_folder.back() != '/')
		local_folder += std::string("/");

	BOOL bResult = FALSE;
	std::vector<std::string> vecobject_path;
	if (list_s3_folder(object_folder, vecobject_path))
	{
		int FindFileCount =(int)vecobject_path.size();
		strLogInfo.Format(_T("[DownloadFolder] OSS文件夹共找到[%d]个文件..."), FindFileCount);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		if (FindFileCount <= 0)
		{
			strLogInfo.Format(_T("[DownloadFolder]: 枚举OSS文件夹中文件失败..."));
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}

		for (size_t i = 0; i < vecobject_path.size(); i++)
		{
			std::string ossfilepath = vecobject_path[i];

			//为了不在指定本地文件夹里再创建一层，故以本地指定文件夹替换掉ossfilepath首个文件夹
			std::string root_ossfolder = object_folder;
			if (root_ossfolder.back() != '/') root_ossfolder += std::string("/");

			std::string root_localfolder = local_folder;
			if (root_localfolder.back() != '/') root_localfolder += std::string("/");

			std::string ossfilepath_ansi = "", root_ossfolder_ansi = root_ossfolder, root_localfolder_ansi = root_localfolder;
			utf8_to_ansi(ossfilepath.c_str(), ossfilepath.length(), ossfilepath_ansi);
			//utf8_to_ansi(root_ossfolder.c_str(), root_ossfolder.length(), root_ossfolder_ansi);
			//utf8_to_ansi(root_localfolder.c_str(), root_localfolder.length(), root_localfolder_ansi); 
			std::string localfile_path = str_replace(ossfilepath_ansi, root_ossfolder_ansi, root_localfolder_ansi);

			CString strFileOSSPath; strFileOSSPath = ossfilepath_ansi.c_str();
			CString strRootOSSPath; strRootOSSPath = root_ossfolder_ansi.c_str();
			CString strLocalRootPath; strLocalRootPath = root_localfolder_ansi.c_str();
			strLogInfo.Format(_T("[DownloadFolder]: OSSFileCount = %d, FileOSSPath = [%s], RootOSSPath = [%s], LocalFolder = [%s]"), (int)vecobject_path.size(), strFileOSSPath.GetBuffer(), strRootOSSPath.GetBuffer(), strLocalRootPath.GetBuffer());
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

			std::string localfile_folder = get_path_folder(localfile_path);
			CString strNowLocalFolder; strNowLocalFolder = localfile_folder.c_str();
			strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx不支持左斜杠
			SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
			if (!::PathFileExists(strNowLocalFolder))
			{
				strLogInfo.Format(_T("[DownloadFolder]: CreateFolder Failed...[%s]"), strNowLocalFolder.GetBuffer());
				WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
				return FALSE;
			}

			CString strNowLocalFile; strNowLocalFile = localfile_path.c_str();
			std::string s_objectfile_path = vecobject_path[i];
			std::string s_localfile_path = localfile_path;
			ansi_to_utf8(s_objectfile_path.c_str(), s_objectfile_path.length(), s_objectfile_path);
			ansi_to_utf8(s_localfile_path.c_str(), s_localfile_path.length(), s_localfile_path);
			if (get_s3_object_multipart(s_objectfile_path, s_localfile_path))
			{
				strLogInfo.Format(_T("[DownloadFolder]: Download Success, localfile:%s"), strNowLocalFile.GetBuffer());
				WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			}
			else
			{
				strLogInfo.Format(_T("[DownloadFolder]: Download Failed, localfile:%s"), strNowLocalFile.GetBuffer());
				WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
				return FALSE;
			}
		}

		bResult = TRUE;
	}

	return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////
//上传文件
bool C_AWSServer::put_s3_object(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path)
{
	CString strLogInfo;
	// 判断文件是否存在
	std::string localfile_path_ansi = localfile_path;
	utf8_to_ansi(localfile_path.c_str(), localfile_path.length(), localfile_path_ansi);
	if (!file_exists(localfile_path_ansi))
	{
		CString strLocalFilePath; strLocalFilePath = localfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object]: 本地文件不存在[ %s ]..."), strLocalFilePath.GetBuffer());
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//配置数据
	std::string s_EndPoint = (CT2A)m_strEndPoint;
	Aws::String aws_EndPoint(s_EndPoint.c_str(), s_EndPoint.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	Aws::String aws_object_path(objectfile_path.c_str(), objectfile_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion;
	strEndpoint = s_EndPoint.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = objectfile_path.c_str(); strRegion = s_region.c_str();
	strLogInfo.Format(_T("[put_s3_object]参数: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_EndPoint;  // S3服务器Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 // 上传文件
	Aws::S3::Model::PutObjectRequest put_object_request;
	const shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", localfile_path_ansi.c_str(), ios_base::in | ios_base::binary);
	put_object_request.SetBucket(aws_bucket);
	put_object_request.SetKey(aws_object_path);
	put_object_request.SetBody(input_data);

	auto put_object_outcome = s3_client.PutObject(put_object_request);
	if (!put_object_outcome.IsSuccess())
	{
		auto error = put_object_outcome.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
		strLogInfo.Format(_T("[put_s3_object]上传文件失败: Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//存储文件对公URL
		//Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
		//std::string sUrl(URL.c_str(), URL.size());urlfile_path = sUrl;
		std::string s_URL = (CT2A)m_strUrl;
		std::string s_WebPrefix = (CT2A)m_strWebPrefix;
		urlfile_path = s_WebPrefix + s_URL + std::string("/") + objectfile_path;

		CString strUrlFilePath; strUrlFilePath = urlfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object]上传文件成功: url:%s"), strUrlFilePath);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}
#endif

	return TRUE;
}
//分段上传
bool C_AWSServer::put_s3_object_multipart(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path)
{
	CString strLogInfo;
	// 判断文件是否存在
	std::string localfile_path_ansi = localfile_path;
	utf8_to_ansi(localfile_path.c_str(), localfile_path.length(), localfile_path_ansi);
	if (!file_exists(localfile_path_ansi))
	{
		CString strLocalFilePath; strLocalFilePath = localfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]: 本地文件不存在[ %s ]..."), strLocalFilePath.GetBuffer());
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//配置数据
	std::string s_EndPoint = (CT2A)m_strEndPoint;
	Aws::String aws_EndPoint(s_EndPoint.c_str(), s_EndPoint.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	Aws::String aws_object_path(objectfile_path.c_str(), objectfile_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion;
	strEndpoint = s_EndPoint.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = objectfile_path.c_str(); strRegion = s_region.c_str();
	strLogInfo.Format(_T("[put_s3_object_multipart]参数: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_EndPoint;  // S3服务器Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 // 分片上传文件

	//1. init
	Aws::String aws_upload_id;
	Aws::S3::Model::CreateMultipartUploadRequest create_multipart_request;
	create_multipart_request.WithBucket(aws_bucket).WithKey(aws_object_path);
	auto createMultiPartResult = s3_client.CreateMultipartUpload(create_multipart_request);
	if (!createMultiPartResult.IsSuccess())
	{
		auto error = createMultiPartResult.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]: CreateMultipartUpload Failed, Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		aws_upload_id = createMultiPartResult.GetResult().GetUploadId();

		CString strUploadID; strUploadID = aws_upload_id.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]: CreateMultipartUpload OK, UploadID: %s"), strUploadID);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}

	//2. uploadPart
	long partSize = 5 * 1024 * 1024;//5M
	std::fstream file(localfile_path_ansi.c_str(), std::ios::in | std::ios::binary);
	file.seekg(0, std::ios::end);
	long fileSize = file.tellg();
	std::cout << file.tellg() << std::endl;
	file.seekg(0, std::ios::beg);

	long filePosition = 0;
	Aws::Vector<Aws::S3::Model::CompletedPart> completeParts;
	char* buffer = new char[partSize]; std::memset(buffer, 0, partSize);

	int partNumber = 1;
	while (filePosition < fileSize)
	{
		partSize = min(partSize, (fileSize - filePosition));
		file.read(buffer, partSize);
		//std::cout << "readSize : " << partSize << std::endl;
		Aws::S3::Model::UploadPartRequest upload_part_request;
		upload_part_request.WithBucket(aws_bucket).WithKey(aws_object_path).WithUploadId(aws_upload_id).WithPartNumber(partNumber).WithContentLength(partSize);

		Aws::String str(buffer, partSize);
		auto input_data = Aws::MakeShared<Aws::StringStream>("UploadPartStream", str);
		upload_part_request.SetBody(input_data);
		filePosition += partSize;

		auto uploadPartResult = s3_client.UploadPart(upload_part_request);
		//std::cout << uploadPartResult.GetResult().GetETag() << std::endl;
		completeParts.push_back(Aws::S3::Model::CompletedPart().WithETag(uploadPartResult.GetResult().GetETag()).WithPartNumber(partNumber));
		std::memset(buffer, 0, partSize);
		++partNumber;
	}

	if (fileSize > 0 && filePosition > 0)
	{
		strLogInfo.Format(_T("[put_s3_object_multipart]: CompletedPart OK, FileSize: %dM, PartCount:%d"), fileSize / (1024 * 1024), partNumber);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}
	else
	{
		strLogInfo.Format(_T("[put_s3_object_multipart]: CompletedPart Falied, FileSize: %dM, PartCount:%d"), fileSize / (1024 * 1024), partNumber);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//3. complete multipart upload
	Aws::S3::Model::CompleteMultipartUploadRequest complete_multipart_request;
	complete_multipart_request.WithBucket(aws_bucket).WithKey(aws_object_path).WithUploadId(aws_upload_id).WithMultipartUpload(Aws::S3::Model::CompletedMultipartUpload().WithParts(completeParts));
	auto completeMultipartUploadResult = s3_client.CompleteMultipartUpload(complete_multipart_request);
	if (!completeMultipartUploadResult.IsSuccess())
	{
		//中断上传
		Aws::S3::Model::AbortMultipartUploadRequest abort_multipart_request;
		abort_multipart_request.WithBucket(aws_bucket).WithKey(aws_object_path).WithUploadId(aws_upload_id);
		s3_client.AbortMultipartUpload(abort_multipart_request);

		//错误信息
		auto error = completeMultipartUploadResult.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]上传文件失败: Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//存储文件对公URL
		//Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
		//std::string sUrl(URL.c_str(), URL.size());urlfile_path = sUrl;
		std::string s_URL = (CT2A)m_strUrl;
		std::string s_WebPrefix = (CT2A)m_strWebPrefix;
		urlfile_path = s_WebPrefix + s_URL + std::string("/") + objectfile_path;

		CString strUrlFilePath; strUrlFilePath = urlfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]上传文件成功: url:%s"), strUrlFilePath);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}

#endif

	return TRUE;
}
//下载文件
bool C_AWSServer::get_s3_object(const std::string objectfile_path, const std::string localfile_path)
{
	CString strLogInfo;
	//配置数据
	std::string s_EndPoint = (CT2A)m_strEndPoint;
	Aws::String aws_EndPoint(s_EndPoint.c_str(), s_EndPoint.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	Aws::String aws_object_path(objectfile_path.c_str(), objectfile_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion;
	strEndpoint = s_EndPoint.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = objectfile_path.c_str(); strRegion = s_region.c_str();
	strLogInfo.Format(_T("[get_s3_object]参数: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_EndPoint;  // S3服务器Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 //下载文件

	//建立请求
	Aws::S3::Model::GetObjectRequest getObjectRequest;
	getObjectRequest.SetBucket(aws_bucket);
	getObjectRequest.SetKey(aws_object_path);

	//下载Object
	Aws::S3::Model::GetObjectOutcome getObjectOutcome = s3_client.GetObject(getObjectRequest);
	if (!getObjectOutcome.IsSuccess())
	{
		//错误信息
		Aws::S3::S3Error error = getObjectOutcome.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
		strLogInfo.Format(_T("[get_s3_object] Download Failed, Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//保存到本地
		std::string localfile_path_ansi = localfile_path;
		utf8_to_ansi(localfile_path.c_str(), localfile_path.length(), localfile_path_ansi);
		CString strLocalFile; strLocalFile = localfile_path_ansi.c_str();
		std::ofstream outputFileStream(localfile_path_ansi, std::ios::binary);
		if (!outputFileStream.is_open())
		{
			strLogInfo.Format(_T("[get_s3_object] OpenFile Failed, local:%s"), strLocalFile);
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}
		outputFileStream << getObjectOutcome.GetResult().GetBody().rdbuf();
		outputFileStream.close();

		strLogInfo.Format(_T("[get_s3_object] Download Success, local:%s"), strLocalFile);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}

#endif

	return TRUE;
}
//分段下载
bool C_AWSServer::get_s3_object_multipart(const std::string objectfile_path, const std::string localfile_path)
{
	CString strLogInfo;
	//配置数据
	std::string s_EndPoint = (CT2A)m_strEndPoint;
	Aws::String aws_EndPoint(s_EndPoint.c_str(), s_EndPoint.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	Aws::String aws_object_path(objectfile_path.c_str(), objectfile_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion;
	strEndpoint = s_EndPoint.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = objectfile_path.c_str(); strRegion = s_region.c_str();
	strLogInfo.Format(_T("[get_s3_object_multipart]参数: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_EndPoint;  // S3服务器Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 //下载文件

	//建立请求
	Aws::S3::Model::GetObjectRequest getObjectRequest;
	getObjectRequest.SetBucket(aws_bucket);
	getObjectRequest.SetKey(aws_object_path);

	//下载Object
	Aws::S3::Model::GetObjectOutcome getObjectOutcome = s3_client.GetObject(getObjectRequest);
	if (!getObjectOutcome.IsSuccess())
	{
		//错误信息
		Aws::S3::S3Error error = getObjectOutcome.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
		strLogInfo.Format(_T("[get_s3_object_multipart] Download Failed, Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		// 获取对象的大小
		long long object_size = getObjectOutcome.GetResult().GetContentLength();

		std::string localfile_path_ansi = localfile_path;
		utf8_to_ansi(localfile_path.c_str(), localfile_path.length(), localfile_path_ansi);
		CString strLocalFile; strLocalFile = localfile_path_ansi.c_str();
		std::ofstream outputFileStream(localfile_path_ansi, std::ios::binary);
		if (!outputFileStream.is_open())
		{
			strLogInfo.Format(_T("[get_s3_object_multipart] OpenFile Failed, local:%s"), strLocalFile);
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}

		// 分段下载并写入本地文件
		long long part_size = 1024 * 1024;									// 每个分段的大小，这里设置为 1MB
		long long num_parts = (object_size + part_size - 1) / part_size;	// 分段的数量
		for (long long i = 0; i < num_parts; i++)
		{
			long long start_byte = i * part_size;
			long long end_byte = (i + 1) * part_size - 1;
			if (end_byte >= object_size)
				end_byte = object_size - 1;

			// 创建GetObjectRequest来指定下载的范围
			Aws::S3::Model::GetObjectRequest partGetObjectRequest;
			partGetObjectRequest.SetBucket(aws_bucket);
			partGetObjectRequest.SetKey(aws_object_path);
			partGetObjectRequest.SetRange("bytes=" + std::to_string(start_byte) + "-" + std::to_string(end_byte));

			// 下载分段
			auto partGetObjectOutcome = s3_client.GetObject(partGetObjectRequest);
			if (!partGetObjectOutcome.IsSuccess())
			{
				//错误信息
				Aws::S3::S3Error error = partGetObjectOutcome.GetError();
				Aws::String aws_ExceptionName = error.GetExceptionName();
				Aws::String aws_Message = error.GetMessage();
				std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
				std::string sMessage(aws_Message.c_str(), aws_Message.size());

				CString strExceptionName, strMessage;
				strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
				strLogInfo.Format(_T("[get_s3_object_multipart] Part Download Failed, Exception:%s, Message:%s"), strExceptionName, strMessage);
				WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
				return FALSE;
			}

			// 将分段内容写入本地文件
			const Aws::IOStream& part_data = partGetObjectOutcome.GetResult().GetBody();
			outputFileStream << part_data.rdbuf();
		}
		outputFileStream.close();

		strLogInfo.Format(_T("[get_s3_object_multipart] Download Success, local:%s"), strLocalFile);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}

#endif

	return TRUE;
}

//枚举OSS目录
bool C_AWSServer::list_s3_folder(const std::string object_folder, std::vector<std::string>& vecobject_name)
{
	CString strLogInfo;
	//配置数据
	std::string s_EndPoint = (CT2A)m_strEndPoint;
	Aws::String aws_EndPoint(s_EndPoint.c_str(), s_EndPoint.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strEndpoint, strAK, strSK, strBucket, strRegion;
	strEndpoint = s_EndPoint.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strRegion = s_region.c_str();
	strLogInfo.Format(_T("[list_s3_folder]参数: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);


	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_EndPoint;  // S3服务器Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 //枚举目录

	//建立请求
	Aws::S3::Model::ListObjectsRequest objectsRequest;
	objectsRequest.SetBucket(aws_bucket);

	std::string s_prefix = object_folder;
	if (s_prefix.back() != '/')
		s_prefix += std::string("/"); //保证只遍历当前文件夹,不遍历父文件夹

	Aws::String aws_prefix(s_prefix.c_str(), s_prefix.size());
	objectsRequest.SetPrefix(aws_prefix);
	CString strPrefix; strPrefix = s_prefix.c_str();
	strLogInfo.Format(_T("[list_s3_folder] Prefix = [%s] "), strPrefix);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//下载Object
	Aws::S3::Model::ListObjectsOutcome list_objects_outcome = s3_client.ListObjects(objectsRequest);
	if (!list_objects_outcome.IsSuccess())
	{
		//错误信息
		Aws::S3::S3Error error = list_objects_outcome.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();

		strLogInfo.Format(_T("[list_s3_folder] Failed to list objects, bucket: %s, prefix: %s, Exception:%s, Message:%s"), strBucket, strPrefix, strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//ListObject会枚举出所有文件,包括子文件夹的文件
		Aws::Vector<Object> objects = list_objects_outcome.GetResult().GetContents();
		for (const auto& object : objects)
		{
			if (object.GetKey().back() == '/') // If it's a directory
			{
				continue;
			}
			else // If it's a file
			{
				std::string object_name = object.GetKey();
				vecobject_name.push_back(object_name);
			}
		}
	}

#endif

	return TRUE;
}
