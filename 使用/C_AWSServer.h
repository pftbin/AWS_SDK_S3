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
	BOOL SetAWSConfig(CString strUrl,CString strAK,CString strSK, CString strBucket, CString strObjectPath, CString strRegion);
	BOOL UploadFile(CString& strLocalFilePath, CString& strUrlFilePath);

protected:
	CString		m_strUrl;
	CString		m_strAK;
	CString		m_strSK;
	CString		m_strBucket;
	CString		m_strObjectFolder;
	CString		m_strObjectName;
	CString		m_strRegion;

protected:
	BOOL put_s3_object(const string& slocalfile_path, string& surlfile_path);
	BOOL put_s3_object_multipart(const string& slocalfile_path, string& surlfile_path);
};

