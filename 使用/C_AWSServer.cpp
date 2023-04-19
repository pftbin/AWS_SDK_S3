#include "stdafx.h"
#include "C_AWSServer.h"

DECLARE_LOGER(LogerRecImportVideo);

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

//判断文件是否存在
inline bool file_exists(const string& name)
{
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
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

static CString NameFromPath(CString& strFullPath, BOOL bExtension)
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

C_AWSServer::C_AWSServer(void)
{
	m_strUrl = _T("");
	m_strAK = _T("");
	m_strSK = _T("");
	m_strBucket = _T("");
	m_strObjectFolder = _T("");
	m_strObjectName = _T("");
	m_strRegion = _T("");
}

C_AWSServer::~C_AWSServer(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////
BOOL C_AWSServer::SetAWSConfig(CString strUrl, CString strAK, CString strSK, CString strBucket, CString strObjectPath, CString strRegion)
{
	if (strUrl.IsEmpty() || strAK.IsEmpty() || strSK.IsEmpty() || strBucket.IsEmpty())
		return FALSE;

	m_strUrl = strUrl;
	m_strAK  = strAK;
	m_strSK  = strSK;
	m_strBucket = strBucket;
	m_strObjectFolder = strObjectPath;
	m_strObjectName = _T("");
	m_strRegion = strRegion;

	return TRUE;
}

BOOL C_AWSServer::UploadFile(CString& strLocalFilePath, CString& strUrlFilePath)
{
	if (strLocalFilePath.IsEmpty())
		return FALSE;

	//调整Object路径
	CString strFullName = NameFromPath(strLocalFilePath,TRUE);
	m_strObjectName = strFullName;

	//上传
	BOOL bResult = FALSE;
	Aws::SDKOptions options;
	Aws::InitAPI(options);

	std::string s_localfile_path = (CT2A)strLocalFilePath;
	std::string s_urlfile_path;
	if (put_s3_object(s_localfile_path, s_urlfile_path))
	{
		bResult = TRUE;
		strUrlFilePath = s_urlfile_path.c_str();
	}

	Aws::ShutdownAPI(options);

	return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////
BOOL C_AWSServer::put_s3_object(const string& slocalfile_path, string& surlfile_path)
{
	// 判断文件是否存在
	if (!file_exists(slocalfile_path))
	{
		CString strLocalFilePath; strLocalFilePath = slocalfile_path.c_str();
		CString strLogInfo;strLogInfo.Format(_T("[put_s3_object]: 本地文件不存在[ %s ]..."), strLocalFilePath);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//配置数据
	std::string s_Url = (CT2A)m_strUrl;
	Aws::String aws_Url(s_Url.c_str(), s_Url.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	CString strObjectPath = m_strObjectFolder +_T("/") + m_strObjectName;
	std::string s_object_path = (CT2A)strObjectPath;
	Aws::String aws_object_path(s_object_path.c_str(), s_object_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strUrl, strAK, strSK, strBucket, strObjPath, strRegion;
	strUrl = s_Url.c_str();strAK = s_AK.c_str();strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = s_object_path.c_str(); strRegion = s_region.c_str();
	CString strLogInfo; strLogInfo.Format(_T("[put_s3_object]上传参数: URL:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strUrl, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);

	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_Url;  // S3服务器地址和端口
	cfg.scheme = Aws::Http::Scheme::HTTP;
	cfg.verifySSL = false;

	Aws::Auth::AWSCredentials cred(aws_AK, aws_SK);  // ak,sk
	Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
	Aws::S3::S3Client s3_client(cred, cfg, signPayloads, false);

#if 1 // 上传文件
	Aws::S3::Model::PutObjectRequest put_object_request;
	const shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", slocalfile_path.c_str(), ios_base::in | ios_base::binary);
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
		strExceptionName = sExceptionName.c_str();strMessage = sMessage.c_str();
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object]上传文件失败: Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//存储文件对公URL
		Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
		std::string sUrl(URL.c_str(), URL.size());

		surlfile_path = sUrl;
		CString strUrlFilePath; strUrlFilePath = surlfile_path.c_str();
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object]上传文件成功: url:%s"), strUrlFilePath);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
	}
#endif

	return TRUE;
}

BOOL C_AWSServer::put_s3_object_multipart(const string& slocalfile_path, string& surlfile_path)
{
	// 判断文件是否存在
	if (!file_exists(slocalfile_path))
	{
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object_multipart]: 本地文件不存在[ %s ]..."), slocalfile_path.c_str());
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//配置数据
	std::string s_Url = (CT2A)m_strUrl;
	Aws::String aws_Url(s_Url.c_str(), s_Url.size());
	std::string s_AK = (CT2A)m_strAK;
	Aws::String aws_AK(s_AK.c_str(), s_AK.size());
	std::string s_SK = (CT2A)m_strSK;
	Aws::String aws_SK(s_SK.c_str(), s_SK.size());
	std::string s_bucket = (CT2A)m_strBucket;
	Aws::String aws_bucket(s_bucket.c_str(), s_bucket.size());
	CString strObjectPath = m_strObjectFolder + _T("/") + m_strObjectName;
	std::string s_object_path = (CT2A)strObjectPath;
	Aws::String aws_object_path(s_object_path.c_str(), s_object_path.size());
	std::string s_region = (CT2A)m_strRegion;
	Aws::String aws_region(s_region.c_str(), s_region.size());

	CString strUrl, strAK, strSK, strBucket, strObjPath, strRegion;
	strUrl = s_Url.c_str(); strAK = s_AK.c_str(); strSK = s_SK.c_str();
	strBucket = s_bucket.c_str(); strObjPath = s_object_path.c_str(); strRegion = s_region.c_str();
	CString strLogInfo; strLogInfo.Format(_T("[put_s3_object_multipart]上传参数: URL:%s, AK:%s, SK:%s, Bucket:%s, ObjPath:%s, Region:%s "), strUrl, strAK, strSK, strBucket, strObjPath, strRegion);
	WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);

	//如果指定了地区
	Aws::Client::ClientConfiguration cfg;
	if (!aws_region.empty())
		cfg.region = aws_region;

	//S3连接
	cfg.endpointOverride = aws_Url;  // S3服务器地址和端口
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
	if (createMultiPartResult.IsSuccess()) 
	{
		aws_upload_id = createMultiPartResult.GetResult().GetUploadId();

		CString strUploadID; strUploadID = aws_upload_id.c_str();
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object_multipart]: CreateMultipartUpload OK, UploadID: %s"), strUploadID);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
	}
	else 
	{
		auto error = createMultiPartResult.GetError();
		Aws::String aws_ExceptionName = error.GetExceptionName();
		Aws::String aws_Message = error.GetMessage();
		std::string sExceptionName(aws_ExceptionName.c_str(), aws_ExceptionName.size());
		std::string sMessage(aws_Message.c_str(), aws_Message.size());
		CString strExceptionName, strMessage;
		strExceptionName = sExceptionName.c_str(); strMessage = sMessage.c_str();

		CString strUploadID; strUploadID = aws_upload_id.c_str();
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object_multipart]: CreateMultipartUpload Failed, Exception:%s, Message:%s"), strUploadID);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}

	//2. uploadPart
	long partSize = 5 * 1024 * 1024;//5M
	std::fstream file(slocalfile_path.c_str(), std::ios::in | std::ios::binary);
	file.seekg(0, std::ios::end);
	long fileSize = file.tellg();
	std::cout << file.tellg() << std::endl;
	file.seekg(0, std::ios::beg);

	long filePosition = 0;
	Aws::Vector<Aws::S3::Model::CompletedPart> completeParts;
	char* buffer = new char[partSize]; memset(buffer, 0, partSize);

	int partNumber = 1;
	while (filePosition < fileSize)
	{
		partSize = min(partSize, (fileSize - filePosition));
		file.read(buffer, partSize);
		std::cout << "readSize : " << partSize << std::endl;
		Aws::S3::Model::UploadPartRequest upload_part_request;
		upload_part_request.WithBucket(aws_bucket).WithKey(aws_object_path).WithUploadId(aws_upload_id).WithPartNumber(partNumber).WithContentLength(partSize);

		Aws::String str(buffer, partSize);
		auto input_data = Aws::MakeShared<Aws::StringStream>("UploadPartStream", str);
		upload_part_request.SetBody(input_data);
		filePosition += partSize;

		auto uploadPartResult = s3_client.UploadPart(upload_part_request);
		std::cout << uploadPartResult.GetResult().GetETag() << std::endl;
		completeParts.push_back(Aws::S3::Model::CompletedPart().WithETag(uploadPartResult.GetResult().GetETag()).WithPartNumber(partNumber));
		memset(buffer, 0, partSize);
		++partNumber;
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
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object_multipart]上传文件失败: Exception:%s, Message:%s"), strExceptionName, strMessage);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
		return FALSE;
	}
	else
	{
		//存储文件对公URL
		Aws::String URL = s3_client.GeneratePresignedUrl(aws_bucket, aws_object_path, Aws::Http::HttpMethod::HTTP_GET);
		std::string sUrl(URL.c_str(), URL.size());

		surlfile_path = sUrl;
		CString strUrlFilePath; strUrlFilePath = surlfile_path.c_str();
		CString strLogInfo; strLogInfo.Format(_T("[put_s3_object_multipart]上传文件成功: url:%s"), strUrlFilePath);
		WRITE_LOG(LogerRecImportVideo, 0, FALSE, _T("%s"), strLogInfo);
	}

#endif

	return TRUE;
}

