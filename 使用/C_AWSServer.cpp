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

//�ж��ļ��Ƿ����
inline bool file_exists(const string& name)
{
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

//�ַ����滻
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

//����ļ���С����λ�ֽ�
inline size_t get_filesize(const char* fileName) 
{
	if (fileName == nullptr) 
		return 0;

	// ����һ���洢�ļ�(��)��Ϣ�Ľṹ�壬�������ļ���С�ʹ���ʱ�䡢����ʱ�䡢�޸�ʱ���
	struct stat statbuf;

	// �ṩ�ļ����ַ���������ļ����Խṹ��
	stat(fileName, &statbuf);

	// ��ȡ�ļ���С
	size_t filesize = statbuf.st_size;

	return filesize;
}

//��·����ȡ�ļ���
std::string get_path_folder(std::string filepath)
{
	std::string folder = "";
	//�����һ����б�ܻ���б�ܴ�����Ŀ¼��
	size_t pos = filepath.find_last_of("/\\");
	if (pos != std::string::npos) {
		folder = filepath.substr(0, pos);
	}
	return folder;
}

//��·����ȡ�ļ���
std::string get_path_name(std::string filepath)
{
	std::string name = "";
	//�����һ����б�ܻ���б�ܴ������ļ���
	size_t pos = filepath.find_last_of("\\/");
	if (pos != std::string::npos)
		name = filepath.substr(pos + 1);

	return name;
}

//��ȡ�ļ��������ļ�
static void GetPathAllFile(CString strDir, std::vector<CString>& vFilePathList)
{
	CFileFind finder;
	BOOL isNotEmpty = finder.FindFile(strDir + _T("*.*"));//���ļ��У���ʼ���� 
	while (isNotEmpty)
	{
		isNotEmpty = finder.FindNextFile();//�����ļ� 
		CString filename = finder.GetFilePath();//��ȡ�ļ���·�����������ļ��У��������ļ� 
		if (!(finder.IsDirectory()))
		{
			//������ļ�������ļ��б� 
			vFilePathList.push_back(filename);//��һ���ļ�·���������� 
		}
		else
		{
			//�ݹ�����û��ļ��У��������û��ļ��� 
			if (!(finder.IsDots() || finder.IsHidden() || finder.IsSystem() || finder.IsTemporary() || finder.IsReadOnly()))
			{
				GetPathAllFile(filename + _T("\\"), vFilePathList);
			}
		}
	}
}
//��ȡ�ļ���
static CString NameFromPath(CString& strFullPath, BOOL bExtension = TRUE)
{
	CString strFinalName;
	if (strFullPath.IsEmpty())
		return strFinalName;

	strFullPath.Replace(_T("\\"), _T("/"));
	CString strFullName = strFullPath.Right(strFullPath.GetLength() - strFullPath.ReverseFind('/') - 1); //����׺���ļ���
	CString strFirstName = strFullName.Left(strFullName.Find('.')); //ȥ����׺
	if (bExtension)
		strFinalName = strFullName;
	else
		strFinalName = strFirstName;

	return strFinalName;
}
//��ȡ�ļ���
static CString FolderFromPath(CString& strFullPath)
{
	CString strFolder = _T("");
	int nPos = strFullPath.ReverseFind('/');
	if (nPos == -1)
		return strFolder;
	strFolder = strFullPath.Left(nPos);//��ȡ�ļ���

	return strFolder;
}

C_AWSServer::C_AWSServer(void)
{
	m_strEndPoint = _T("");
	m_strAK = _T("");
	m_strSK = _T("");
	m_strBucket = _T("");
	m_strRegion = _T("");

	Aws::InitAPI(m_aws_options);
}

C_AWSServer::~C_AWSServer(void)
{
	Aws::ShutdownAPI(m_aws_options);
}

////////////////////////////////////////////////////////////////////////////////////////
BOOL C_AWSServer::SetAWSConfig(CString strEndPoint, CString strUrl, CString strAK, CString strSK, CString strBucket, CString strRegion)
{
	if (strEndPoint.IsEmpty() || strUrl.IsEmpty() || strAK.IsEmpty() || strSK.IsEmpty() || strBucket.IsEmpty())
		return FALSE;

	m_strEndPoint = strEndPoint;
	m_strUrl = strUrl;
	m_strAK  = strAK;
	m_strSK  = strSK;
	m_strBucket = strBucket;
	m_strRegion = strRegion;

	return TRUE;
}

BOOL C_AWSServer::UploadFile(CString& strObjectFolder, CString& strLocalFilePath, CString& strUrlFilePath)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[UploadFile] OSS·��[ %s ],�����ļ�[ %s ]"), strObjectFolder.GetBuffer(), strLocalFilePath.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	
	if (strLocalFilePath.IsEmpty())
		return FALSE;
	strObjectFolder.Replace(_T("\\"), _T("/"));
	strLocalFilePath.Replace(_T("\\"), _T("/"));

	//Object·��
	CString strFullName = NameFromPath(strLocalFilePath,TRUE);
	if (strObjectFolder.Right(1) != _T("/"))
		strObjectFolder += _T("/");
	CString strObjectFilePath = strObjectFolder + strFullName;

	//�ϴ�
	BOOL bResult = FALSE;
	std::string s_objectfile_path = (CT2A)strObjectFilePath;
	std::string s_localfile_path = (CT2A)strLocalFilePath;
	std::string s_urlfile_path;
	for (int i = 0; i < DF_RETRY_TIMES; i++)
	{
		if (put_s3_object_multipart(s_objectfile_path, s_localfile_path, s_urlfile_path))
		{
			bResult = TRUE;
			strUrlFilePath = s_urlfile_path.c_str();
			break;
		}
	}

	return bResult;
}

BOOL C_AWSServer::UploadFolder(CString& strObjectFolder, CString& strLocalFolder)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[UploadFolder] OSS·��[ %s ],�����ļ���[ %s ]"), strObjectFolder.GetBuffer(), strLocalFolder.GetBuffer());
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
		CString localfile_path = vecFilePath[i]; localfile_path.Replace(_T("\\"), _T("/"));		//�ļ���·������ D:/AAA/BBB/123.mp4
		CString local_filefolder = FolderFromPath(localfile_path);								//�ļ���·������ D:/AAA/BBB

		CString object_fullfolder = strObjectFolder;
		CString object_subfolder = local_filefolder; 
		object_subfolder.Replace(strLocalFolder, _T(""));										//���ļ���·����ɾ����Ŀ¼���� D:/AAA
		if (object_subfolder.GetLength() > 1)													//��󳤶ȴ���1������������ļ��У��� /BBB
			object_fullfolder += object_subfolder;

		if (!UploadFile(object_fullfolder, localfile_path, urlfile_path))
			bResult &= FALSE;
	}

	return bResult;
}

BOOL C_AWSServer::DownloadFile(CString strObjectFilePath, CString strLocalFilePath)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[DownloadFile] OSS�ļ�·��[ %s ],�����ļ�[ %s ]"), strObjectFilePath.GetBuffer(), strLocalFilePath.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	if (strObjectFilePath.IsEmpty() || strLocalFilePath.IsEmpty())
		return FALSE;
	strObjectFilePath.Replace(_T("\\"), _T("/"));
	strLocalFilePath.Replace(_T("\\"), _T("/"));

	CString strLocalFolder = FolderFromPath(strLocalFilePath);
	strLogInfo.Format(_T("[DownloadFile] ���������ļ���[%s]"), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (!::PathFileExists(strLocalFolder))
	{
		CString strNowLocalFolder = strLocalFolder;
		strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx��֧����б��
		::SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
		if (!::PathFileExists(strNowLocalFolder))
			return FALSE;
	}

	//����
	BOOL bResult = FALSE;
	std::string objectfile_path = (CT2A)strObjectFilePath;
	std::string localfile_path = (CT2A)strLocalFilePath;
	for (int i = 0; i < DF_RETRY_TIMES; i++)
	{
		if (get_s3_object(objectfile_path, localfile_path))
		{
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

BOOL C_AWSServer::DownloadFolder(CString& strObjectFolder, CString& strLocalFolder)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[DownloadFolder] OSS·��[ %s ],�����ļ���[ %s ]"), strObjectFolder.GetBuffer(), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (strObjectFolder.IsEmpty() || strLocalFolder.IsEmpty())
		return FALSE;
	strObjectFolder.Replace(_T("\\"), _T("/"));
	strLocalFolder.Replace(_T("\\"), _T("/"));

	strLogInfo.Format(_T("[DownloadFolder] ���������ļ���[%s]"), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (!::PathFileExists(strLocalFolder))
	{
		CString strNowLocalFolder = strLocalFolder; 
		strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx��֧����б��
		::SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
		if (!::PathFileExists(strNowLocalFolder)) 
			return FALSE;
	}
	std::string object_folder = (CT2A)strObjectFolder;
	std::string local_folder = (CT2A)strLocalFolder;
	if (local_folder.back() != '/')
		local_folder += std::string("/");

	BOOL bResult = FALSE;
	std::vector<std::string> vecobject_path;
	if (list_s3_folder(object_folder, vecobject_path))
	{
		for (size_t i = 0; i < vecobject_path.size(); i++)
		{
			//std::string localfile_path = local_folder + ossfilepath;
			
			//Ϊ�˲���ָ�������ļ������ٴ���һ�㣬���Ա���ָ���ļ����滻��ossfilepath�׸��ļ���
			std::string root_ossfolder = object_folder;
			if (root_ossfolder.back() != '/')
				root_ossfolder += std::string("/");
			std::string ossfilepath = vecobject_path[i];
			std::string localfile_path = str_replace(ossfilepath, root_ossfolder, local_folder);

			std::string localfile_folder = get_path_folder(localfile_path);
			CString strNowLocalFolder; strNowLocalFolder = localfile_folder.c_str();
			strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx��֧����б��
			SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
			if (!::PathFileExists(strNowLocalFolder))
			{
				strLogInfo.Format(_T("[DownloadFolder]: CreateFolder Failed, folder:%s"), strNowLocalFolder.GetBuffer());
				WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
				return FALSE;
			}

			CString strNowLocalFile; strNowLocalFile = localfile_path.c_str();
			if (get_s3_object(vecobject_path[i], localfile_path))
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

BOOL C_AWSServer::GetHttpFilePath(CString strObjectFilePath, CString& strUrlFilePath)
{
	strUrlFilePath = _T("https://") + m_strUrl + _T("/") + strObjectFilePath;
	return TRUE;

	CString strLogInfo;
	//��������
	std::string s_EndPoint = (CT2A)m_strEndPoint;
	Aws::String aws_EndPoint(s_EndPoint.c_str(), s_EndPoint.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	std::string objectfile_path = (CT2A)strObjectFilePath;
	Aws::String aws_object_path(objectfile_path.c_str(), objectfile_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion;
	strEndpoint = s_EndPoint.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = objectfile_path.c_str(); strRegion = s_region.c_str();
	strLogInfo.Format(_T("[put_s3_object]����: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//���ָ���˵���
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3����
	cfg.endpointOverride = aws_EndPoint;  // S3������Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

	//�洢�ļ��Թ�URL
	Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
	std::string sUrl(URL.c_str(), URL.size());
	std::string urlfile_path = sUrl;strUrlFilePath = urlfile_path.c_str();

	return TRUE;
}

//�޷�ö��Folder��ʱ���Ӻ�������������ԭʼ��Ƶ�ļ���
BOOL C_AWSServer::DownloadVideoFolder(CString& strObjectFolder, CString& strLocalFolder)
{
	CString strLogInfo;
	strLogInfo.Format(_T("[DownloadVideoFolder] OSS·��[ %s ],�����ļ���[ %s ]"), strObjectFolder.GetBuffer(), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (strObjectFolder.IsEmpty() || strLocalFolder.IsEmpty())
		return FALSE;
	strObjectFolder.Replace(_T("\\"), _T("/"));
	strLocalFolder.Replace(_T("\\"), _T("/"));

	strLogInfo.Format(_T("[DownloadVideoFolder] ���������ļ���[%s]"), strLocalFolder.GetBuffer());
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	if (!::PathFileExists(strLocalFolder))
	{
		CString strNowLocalFolder = strLocalFolder;
		strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx��֧����б��
		::SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
		if (!::PathFileExists(strNowLocalFolder))
			return FALSE;
	}
	std::string object_folder = (CT2A)strObjectFolder;
	std::string local_folder = (CT2A)strLocalFolder;
	if (local_folder.back() != '/')
		local_folder += std::string("/");

	//���ٲ�ѯ,ֱ��������ݵ�vector
	std::string old_object_file = object_folder + std::string(".mp4");
	std::string mp4file_name = get_path_name(old_object_file);
	std::string avifile_name = str_replace(mp4file_name, ".mp4", ".avi");
	std::string h5file_name = str_replace(mp4file_name, ".mp4", ".h5");
	std::string pngfile_name = str_replace(mp4file_name, ".mp4", "ori.png");
	std::string folder_name = str_replace(mp4file_name, ".mp4", "");

	std::vector<std::string> vecobject_path;
	if (object_folder.back() != '/')
		object_folder += std::string("/");
	std::string avifile_path = object_folder + avifile_name;  vecobject_path.push_back(avifile_path);
	std::string h5file_path = object_folder + h5file_name;    vecobject_path.push_back(h5file_path);
	std::string pngfile_path = object_folder + pngfile_name;    vecobject_path.push_back(pngfile_path);

	BOOL bResult = FALSE;
	for (size_t i = 0; i < vecobject_path.size(); i++)
	{
		std::string localfile_path = local_folder + get_path_name(vecobject_path[i]);
		std::string localfile_folder = get_path_folder(localfile_path);

		CString strNowLocalFolder; strNowLocalFolder = localfile_folder.c_str();
		strNowLocalFolder.Replace(_T("/"), _T("\\"));//SHCreateDirectoryEx��֧����б��
		SHCreateDirectoryEx(NULL, strNowLocalFolder, NULL);
		if (!::PathFileExists(strNowLocalFolder))
		{
			strLogInfo.Format(_T("[DownloadVideoFolder]: CreateFolder Failed, folder:%s"), strNowLocalFolder.GetBuffer());
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}

		CString strNowLocalFile; strNowLocalFile = localfile_path.c_str();
		if (get_s3_object(vecobject_path[i], localfile_path))
		{
			strLogInfo.Format(_T("[DownloadVideoFolder]: Download Success, localfile:%s"), strNowLocalFile.GetBuffer());
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		}
		else
		{
			strLogInfo.Format(_T("[DownloadVideoFolder]: Download Failed, localfile:%s"), strNowLocalFile.GetBuffer());
			WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
			return FALSE;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//�ϴ��ļ�
bool C_AWSServer::put_s3_object(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path)
{
	CString strLogInfo;
	// �ж��ļ��Ƿ����
	if (!file_exists(localfile_path))
	{
		CString strLocalFilePath; strLocalFilePath = localfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object]: �����ļ�������[ %s ]..."), strLocalFilePath.GetBuffer());
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//��������
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
	strLogInfo.Format(_T("[put_s3_object]����: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//���ָ���˵���
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3����
	cfg.endpointOverride = aws_EndPoint;  // S3������Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 // �ϴ��ļ�
	Aws::S3::Model::PutObjectRequest put_object_request;
	const shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", localfile_path.c_str(), ios_base::in | ios_base::binary);
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
		strLogInfo.Format(_T("[put_s3_object]�ϴ��ļ�ʧ��: Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//�洢�ļ��Թ�URL
		//Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
		//std::string sUrl(URL.c_str(), URL.size());urlfile_path = sUrl;
		std::string s_URL = (CT2A)m_strUrl;
		urlfile_path = std::string("https://") + s_URL + std::string("/") + objectfile_path;

		CString strUrlFilePath; strUrlFilePath = urlfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object]�ϴ��ļ��ɹ�: url:%s"), strUrlFilePath);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}
#endif

	return TRUE;
}
//�ֶ��ϴ�
bool C_AWSServer::put_s3_object_multipart(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path)
{
	CString strLogInfo;
	// �ж��ļ��Ƿ����
	if (!file_exists(localfile_path))
	{
		CString strLocalFilePath; strLocalFilePath = localfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]: �����ļ�������[ %s ]..."), strLocalFilePath.GetBuffer());
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//��������
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
	strLogInfo.Format(_T("[put_s3_object_multipart]����: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//���ָ���˵���
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3����
	cfg.endpointOverride = aws_EndPoint;  // S3������Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 // ��Ƭ�ϴ��ļ�

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
	std::fstream file(localfile_path.c_str(), std::ios::in | std::ios::binary);
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
		//�ж��ϴ�
		Aws::S3::Model::AbortMultipartUploadRequest abort_multipart_request;
		abort_multipart_request.WithBucket(aws_bucket).WithKey(aws_object_path).WithUploadId(aws_upload_id);
		s3_client.AbortMultipartUpload(abort_multipart_request);

		//������Ϣ
		auto error = completeMultipartUploadResult.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());

		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]�ϴ��ļ�ʧ��: Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//�洢�ļ��Թ�URL
		//Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
		//std::string sUrl(URL.c_str(), URL.size());urlfile_path = sUrl;
		std::string s_URL = (CT2A)m_strUrl;
		urlfile_path = std::string("https://") + s_URL + std::string("/") + objectfile_path;

		CString strUrlFilePath; strUrlFilePath = urlfile_path.c_str();
		strLogInfo.Format(_T("[put_s3_object_multipart]�ϴ��ļ��ɹ�: url:%s"), strUrlFilePath);
		WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);
	}

#endif

	return TRUE;
}
//�����ļ�
bool C_AWSServer::get_s3_object(const std::string objectfile_path, const std::string localfile_path)
{
	CString strLogInfo;
	//��������
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
	strLogInfo.Format(_T("[get_s3_object]����: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);

	//���ָ���˵���
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3����
	cfg.endpointOverride = aws_EndPoint;  // S3������Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 //�����ļ�

	//��������
	Aws::S3::Model::GetObjectRequest getObjectRequest;
	getObjectRequest.SetBucket(aws_bucket);
	getObjectRequest.SetKey(aws_object_path);

	//����Object
	Aws::S3::Model::GetObjectOutcome getObjectOutcome = s3_client.GetObject(getObjectRequest);
	if (!getObjectOutcome.IsSuccess())
	{
		//������Ϣ
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
		//���浽����
		CString strLocalFile; strLocalFile = localfile_path.c_str();
		std::ofstream outputFileStream(localfile_path, std::ios::binary);
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
//ö��OSSĿ¼
bool C_AWSServer::list_s3_folder(const std::string object_folder, std::vector<std::string>& vecobject_name)
{
	CString strLogInfo;
	//��������
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
	strLogInfo.Format(_T("[get_s3_object]����: EndPoint:%s, AK:%s, SK:%s, Bucket:%s, Region:%s "), strEndpoint, strAK, strSK, strBucket, strRegion);
	WRITE_LOG(LogerDVInterface, 0, FALSE, _T("%s"), strLogInfo);


	//���ָ���˵���
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3����
	cfg.endpointOverride = aws_EndPoint;  // S3������Endpoint
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 //ö��Ŀ¼

	//��������
	Aws::S3::Model::ListObjectsRequest objectsRequest;
	objectsRequest.SetBucket(aws_bucket);
	std::string s_prefix = object_folder;
	if (s_prefix.back() == '/')//���ļ���
		s_prefix.erase(s_prefix.length() - 1, 1); // ɾ��ĩβ����б�ܲ��ܱ���

	Aws::String aws_prefix(s_prefix.c_str(), s_prefix.size());
	objectsRequest.SetPrefix(aws_prefix);
	CString strPrefix; strPrefix = s_prefix.c_str();

	//����Object
	Aws::S3::Model::ListObjectsOutcome list_objects_outcome = s3_client.ListObjects(objectsRequest);
	if (!list_objects_outcome.IsSuccess())
	{
		//������Ϣ
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
		//ListObject��ö�ٳ������ļ�,�������ļ��е��ļ�
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

