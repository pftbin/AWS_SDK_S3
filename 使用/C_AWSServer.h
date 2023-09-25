#pragma once

//AWS
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentialsProvider.h"
#include "aws/s3/S3Client.h"
#include "aws/s3/model/PutObjectRequest.h"
#include "aws/s3/model/GetObjectRequest.h"
//分段上传
#include "aws/core/utils/HashingUtils.h"
#include "aws/s3/model/CreateMultipartUploadRequest.h"
#include "aws/s3/model/CompleteMultipartUploadRequest.h"
#include "aws/s3/model/UploadPartRequest.h"
#include "aws/s3/model/AbortMultipartUploadRequest.h"

//遍历目录
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

#include "Interface_Define.h"
class C_AWSServer : public I_OSSHandler
{
public:
	C_AWSServer(void);
	virtual ~C_AWSServer(void);

public:
	virtual BOOL IsEnable();
	BOOL SetAWSConfig(CString strWebPrefix, CString strEndPoint,CString strUrl,CString strAK,CString strSK, CString strBucket, CString strRegion);

	virtual BOOL UploadFile(CString strObjectFolder, CString strLocalFilePath, CString& strUrlFilePath);
	virtual BOOL UploadFolder(CString strObjectFolder, CString strLocalFolder);

	virtual BOOL DownloadFile(CString strObjectFilePath, CString strLocalFilePath);
	virtual BOOL DownloadFolder(CString strObjectFolder, CString strLocalFolder);

protected:
	CString		m_strWebPrefix;
	CString     m_strEndPoint;
	CString		m_strUrl;
	CString		m_strAK;
	CString		m_strSK;
	CString		m_strBucket;
	CString		m_strRegion;
	BOOL        m_bOssConfig;
protected:
	Aws::SDKOptions		m_aws_options;
	bool put_s3_object(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path);
	bool put_s3_object_multipart(const std::string objectfile_path, const std::string localfile_path, std::string& urlfile_path);
	bool get_s3_object(const std::string objectfile_path, const std::string localfile_path);
	bool get_s3_object_multipart(const std::string objectfile_path, const std::string localfile_path);
	bool list_s3_folder(const std::string object_folder, std::vector<std::string>& vecobject_name);
};

