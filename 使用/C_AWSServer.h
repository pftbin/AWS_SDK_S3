#pragma once

//AWS
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentialsProvider.h"
#include "aws/s3/S3Client.h"
#include "aws/s3/model/PutObjectRequest.h"
#include "aws/s3/model/GetObjectRequest.h"
//multipart
#include "aws/core/utils/HashingUtils.h"
#include "aws/s3/model/CreateMultipartUploadRequest.h"
#include "aws/s3/model/CompleteMultipartUploadRequest.h"
#include "aws/s3/model/UploadPartRequest.h"
#include "aws/s3/model/AbortMultipartUploadRequest.h"
//±éÀúÄ¿Â¼
#include "aws/s3/model/ListObjectsRequest.h"
#include "aws/s3/model/ListObjectsResult.h"
#include "aws/s3/model/ListObjectsV2Request.h"
#include "aws/s3/model/ListObjectsV2Result.h"
#include "aws/s3/model/Object.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
using namespace std;
using namespace Aws::S3::Model;

class C_AWSServer
{
public:
	C_AWSServer(void);
	virtual ~C_AWSServer(void);

public:
	BOOL SetAWSConfig(CString strEndPoint,CString strUrl,CString strAK,CString strSK, CString strBucket, CString strRegion);

	BOOL UploadFile(CString& strObjectFolder, CString& strLocalFilePath, CString& strUrlFilePath);
	BOOL UploadFolder(CString& strObjectFolder, CString& strLocalFolder);

	BOOL DownloadFile(CString strObjectFilePath, CString strLocalFilePath);
	BOOL DownloadFolder(CString& strObjectFolder, CString& strLocalFolder);

	BOOL GetHttpFilePath(CString strObjectFilePath, CString& strUrlFilePath);
	BOOL DownloadVideoFolder(CString& strObjectFolder, CString& strLocalFolder);


protected:
	CString     m_strEndPoint;
	CString		m_strUrl;
	CString		m_strAK;
	CString		m_strSK;
	CString		m_strBucket;
	CString		m_strRegion;

protected:
	Aws::SDKOptions		m_aws_options;
	bool put_s3_object(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path);
	bool put_s3_object_multipart(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path);
	bool get_s3_object(const std::string objectfile_path, const std::string localfile_path);
	bool list_s3_folder(const std::string object_folder, std::vector<std::string>& vecobject_name);
};

