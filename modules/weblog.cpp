/*
 * Copyright (C) 2004-2020 ZNC, see the NOTICE file for details.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <znc/IRCNetwork.h>
#include <znc/User.h>
#include <znc/znc.h>
#include <sys/stat.h>

using std::vector;

class CWebLog : public CModule {
  public:
	MODCONSTRUCTOR(CWebLog) {}

	~CWebLog() override {}
	vector<CString> m_vFiles;

	/**
	 * GetWebMenuTitle
	 * Return the name of this module for use in the side bar
	 */

	virtual CString GetWebMenuTitle() override { return "Log Viewer"; }

	/**
	 * OnWebRequest
	 * Handle all the web requests and page requests for this module
	 * @param WebSock The active request
	 * @param sPageName The name of the page that has been requested
	 * @param Tmpl The active template for adding variables and loops
	 */

	bool OnWebRequest(CWebSock& WebSock, const CString& sPageName, CTemplate& Tmpl) override {
		std::shared_ptr<CWebSession> spSession = WebSock.GetSession();
		CString sUsername = WebSock.GetUser();
		CString sDirectory = WebSock.GetParam("dir", false);

		if (sPageName.AsLower() == "index") {
			if (!WebSock.GetParam("scope", true).empty()) {
				CString sScope = WebSock.GetRawParam("scope", true);
				SetScope(sScope, WebSock, Tmpl);
			}

			if (GetNV(sUsername).empty()) {
				Tmpl["noscope"] = "true";
				spSession->AddError("Error: No scope set. Please set a scope below.");	
			} else {
				ListDir(Tmpl, sDirectory, WebSock);
			}
		} else if (sPageName.AsLower() == "log" || sPageName.AsLower() == "raw") {
			ViewLog(Tmpl, sDirectory, WebSock, sPageName);
		} else if (sPageName.AsLower() == "download") {
			DownloadLog(Tmpl, sDirectory, WebSock);
		}

		GetScopes(WebSock, Tmpl);
		return true;
	}

	/**
	 * WebRequiresLogin
	 * Check whether this module requires the user to be logged in to use the web side of the module
	 */

	bool WebRequiresLogin() override {
		return true;
	}

	/**
	 * ValidateWebRequestCSRFCheck
	 * Performs a cross-site request forgery check
	 * @param WebSock The active request
	 * @param sPageName The name of the page that has been requested
	 */

	bool ValidateWebRequestCSRFCheck(CWebSock& WebSock,
		const CString& sPageName) override {
		return true;
	}

	/**
	 * ListDirectory
	 * Lists the contents of a hiven directory and outputs to to a vector
	 * @param sDirectory The directory to list the contents of
	 */

	vector<CString>& ListDirectory(const CString& sDirectory) {
		const char* sDir = sDirectory.c_str();
		printf (std::string(sDirectory).c_str());
		m_vFiles.clear();
		CDir Dir = sDirectory;

		if(CFile::Exists(sDirectory)) {
			for (unsigned int a = 0; a < Dir.size(); a++) {
				CFile& File = *Dir[a];
				m_vFiles.push_back(File.GetShortName());
				const char* sFile = File.GetShortName().c_str();
				printf (std::string(sFile).c_str());
			}
		}
		return m_vFiles;
	}

	/**
	 * GetBase
	 * Retrieve the base path for a given username retrieved using websock and environment variables
	 * @param WebSock The web socket handle being used by the user/session
	 */

	CString GetBase(CWebSock& WebSock) {
		CString sBase = CZNC::Get().GetZNCPath();
		CString sUsername = WebSock.GetUser();
		CString sScope = GetNV(sUsername);

		if (sScope.AsLower() == "global") {
			sBase = sBase + "/moddata/log/" + sUsername + "/";
		} else if (sScope.AsLower() == "user") {
			sBase = sBase + "/users/" + sUsername + "/moddata/log/";
		} else {
			sBase = sBase + "/users/" + sUsername + "/networks/" + sScope + "/moddata/log/";
		}

		return sBase;
	}

	/**
	 * SetScope
	 * Set the scope for the current user/session
	 * @param sScope The scope to be set and saved to the environment variable
	 * @param WebSock The web socket handle being used by the user/session
	 * @param Tmpl The web template handle being used by the user/session
	 */

	void SetScope(CString& sScope, CWebSock& WebSock, CTemplate& Tmpl) {
		CString sUsername = WebSock.GetUser();
		SetNV(sUsername, sScope);
		CTemplate& Row = Tmpl.AddRow("MessageLoop");
		Row["message"] = "Scope successfully set.";
	}

	/**
	 * GetScopes
	 * Generate the list of scopes available to the user and return them templated
	 * @param WebSock The web socket handle being used by the user/session
	 * @param Tmpl The web template handle being used by the user/session
	 */

	void GetScopes(CWebSock& WebSock, CTemplate& Tmpl) {
		CString sUsername = WebSock.GetUser();
		CUser* pUser = CZNC::Get().FindUser(sUsername);
		vector<CString> m_vsNetworks;

		if (pUser) {
			const vector<CIRCNetwork*>& vNetworks = pUser->GetNetworks();

			for (const CIRCNetwork* pNetwork : vNetworks) {
				m_vsNetworks.push_back(pNetwork->GetName());
			}
		}

		m_vsNetworks.push_back("Global");
		m_vsNetworks.push_back("User");

		for (const CString sNetwork : m_vsNetworks) {
			CTemplate& Row = Tmpl.AddRow("ScopeLoop");
			if (sNetwork == GetNV(sUsername)) {
				Row["active"] = "True";
			}
			Row["network"] = sNetwork;			
		}
	}

	/**
	 * BreadCrumbs
	 * Generate the BreadCrumbs with text and links showing the log directory we are viewing
	 * @param Tmpl The web template handle being used by the user/session
	 * @param sDir The directory we are currently viewing
	 * @param bIsLog True or False depending on whether this is a log file or a log directory
	 */

	void BreadCrumbs(CTemplate& Tmpl, CString& sDir, bool bIsLog) {
		VCString vsDirectories;
		CString sFullURL;
		sDir.Split("/", vsDirectories);

		CTemplate& Row = Tmpl.AddRow("BreadcrumbLoop");
		Row["crumbtext"] = "logs";
		Row["crumburl"] = "";

		if(vsDirectories.size() > 0) {
			unsigned int a = 0;
			for (const CString& sPart : vsDirectories) {
				a++;
				CTemplate& l = Tmpl.AddRow("BreadcrumbLoop");
				l["crumbtext"] = sPart;
				sPart.Replace_n("#", "%23");
				sFullURL.append("/" + sPart);
				l["crumburl"] = sFullURL;
				if (a = vsDirectories.size()-1 && bIsLog == true) {
					l["islog"] = "True";
				}
			}
		}
	}

	/**
	 * ListDir
	 * Retrieve the contents of a given directory and return the results templated
	 * @param WebSock The web socket handle being used by the user/session
	 * @param sDir The directory we would like to obtain the list from
	 * @param Tmpl The web template handle being used by the user/session
	 */

	void ListDir(CTemplate& Tmpl, CString& sDir, CWebSock& WebSock) {
		CString sBase = GetBase(WebSock);
		CString sUsername = WebSock.GetUser();
		CString sFullPath = sBase + sDir;

		if(!IsAllowedPath(sUsername, sFullPath)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "You do not have permission to list directory '/logs" + sDir + "'";
			return;			
		}

		if (!CFile::Exists(sBase + sDir)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Directory '/logs" + sDir + "' does not exist";
			return;
		}

		if (CFile::IsReg(sBase + sDir)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Directory '/logs" + sDir + "' is a file";
			return;
		}

		vector<CString> m_vLogs = ListDirectory(sBase + sDir);

		BreadCrumbs(Tmpl, sDir, false);

		if(m_vLogs.size() > 0) {
			unsigned int a = 0;
			for (vector<CString>::iterator it = m_vLogs.begin(); it != m_vLogs.end(); ++it, a++) {
				CTemplate& Row = Tmpl.AddRow("ListLoop");
				CString sPath;
				if (sDir.empty()) {
					sPath = (*it);
				} else {
					sPath = sDir + "/" + (*it);
				}

				CString sURL;
				off_t sSize = GetSize(sBase + sPath);
				time_t iMTime = CFile::GetMTime(sBase + sPath);

				if(CFile::IsDir(sBase + sPath)) {
					sURL = "?dir=" + sPath;
					sURL = sURL.Replace_n("#", "%23");
				} else {
					sURL = "log?dir=" + sPath;
					sURL = sURL.Replace_n("#", "%23");
				}

				Row["scope"] = sURL;
				Row["item"] = (*it);
				Row["size"] = CString(sSize/1024) + " KB";
				Row["modified"] = WebSock.GetDate(iMTime);
			}			
		} else {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Directory '" + sDir + "' is empty";
		}
		
	}

	/**
	 * ViewLog
	 * Retrieve the contents of a log file and return the results templated
	 * @param Tmpl The web template handle being used by the user/session
	 * @param sDir The directory path of the log file we would like to view
	 * @param WebSock The web socket handle being used by the user/session
	 * @param sPageName The name of the page we are viewing. Used to determine whether we should show the raw or templated log
	 */

	bool ViewLog(CTemplate& Tmpl, CString& sDir, CWebSock& WebSock, const CString& sPageName)
	{
		CString sLine;
		CString sUsername = WebSock.GetUser();
		CString sBase = GetBase(WebSock);
		CString sPath = sBase + sDir;

		if(!IsAllowedPath(sUsername, sPath)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "You do not have permission to view '/logs" + sDir + "'";
			return true;			
		}
 
		CTemplate& Row = Tmpl.AddRow("LogLoop");	

		if (!CFile::Exists(sPath)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "File " + sDir + " does not exist";
			return true;
		}

		CFile pFile(sPath);

		if (pFile.IsDir()) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "File '" + sDir + "' is a directory";
			return true;
		}	

	
		if(!pFile.Open(int(O_RDONLY), mode_t(0600))) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Unable to open file '" + sDir + "'";
			return true;
		}

		if (!pFile.Seek(0)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Unable to read file '" + sDir + "'";
			return true;
		}

		CString sLog;
		while (pFile.ReadLine(sLine)) {
			sLog.append(sLine);
		}	
		pFile.Close();

		if (sPageName.AsLower() == "raw") {
			sLog = sLog.Replace_n("<", "&lt;").Replace_n(">", "&gt;");
		}
		Row["log"] = sLog;
		if (sPageName.AsLower() == "log") {
			BreadCrumbs(Tmpl, sDir, true);
			Row["raw"] = "raw?dir=" + sDir.Replace_n("#", "%23");
			Row["download"] = "download?dir=" + sDir.Replace_n("#", "%23");
		}
		return true;
	}

	/**
	 * DownloadLog
	 * Retrieve the contents of a log file and prepare the file for download
	 * @param Tmpl The web template handle being used by the user/session
	 * @param sDir The directory path of the log file we would like to download
	 * @param WebSock The web socket handle being used by the user/session
	 */

	bool DownloadLog(CTemplate& Tmpl, CString& sDir, CWebSock& WebSock)
	{
		CString sLine;
		CString sUsername = WebSock.GetUser();
		CString sBase = GetBase(WebSock);
		CString sPath = sBase + sDir;
		VCString vsDirectories;
		sDir.Split("/", vsDirectories);
		CString sFilename = vsDirectories[vsDirectories.size()-1];

		if(!IsAllowedPath(sUsername, sPath)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "You do not have permission to view '/logs" + sDir + "'";
			return true;			
		}
 
		CTemplate& Row = Tmpl.AddRow("LogLoop");	

		CFile pFile(sPath);

		if (!CFile::Exists(sPath)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "File " + sDir + " does not exist";
			return true;
		}

		if (pFile.IsDir()) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "File '" + sDir + "' is a directory";
			return true;
		}	

	
		if(!pFile.Open(int(O_RDONLY), mode_t(0600))) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Unable to open file '" + sDir + "'";
			return true;
		}

		if (!pFile.Seek(0)) {
			CTemplate& Row = Tmpl.AddRow("LogErrorLoop");
			Row["Error"] = "Unable to read file '" + sDir + "'";
			return true;
		}

		CString sLog;
		off_t sSize = GetSize(sPath);
		time_t iMTime = pFile.GetMTime();
		WebSock.AddHeader("Content-Transfer-Encoding", "binary");
		WebSock.AddHeader("Last Modified", WebSock.GetDate(iMTime));
		WebSock.AddHeader("Accept-Ranges", "bytes");
		WebSock.AddHeader("Content-Length", CString(sSize));
		WebSock.AddHeader("Content-Encoding", "none");
		WebSock.AddHeader("Content-Type", "text/plain");
		WebSock.AddHeader("Content-Disposition", "attachment; filename=" + sFilename);
		while (pFile.ReadLine(sLine)) {
			sLog.append(sLine);
		}	
		pFile.Close();
		Row["log"] = sLog;

		return true;
	}

	/**
	 * IsAllowedPath
	 * Check whether the full path provided is a safe and valid path
	 * @param sUsername The username of the web session user
	 * @param sPath The path to be checked
	 */

	bool IsAllowedPath(CString& sUsername, CString& sPath) {
		if((!sPath.StartsWith(CZNC::Get().GetZNCPath())) || 
		(!sPath.Contains(sUsername)) || 
		(sPath.Contains("/.."))) return false;
		return true;
	}

	/**
	 * GetSize
	 * Retrieve the size of a given file or directory (Based on CFile::GetSize)
	 * @param sFile The name of the file or directory to retrieve the size of
	 */

	off_t GetSize(const CString& sFile) {
		struct stat st;
		unsigned int iDirSize = 0;
	
		if(CFile::IsDir(sFile)) {
			vector<CString> m_vSubDir = ListDirectory(sFile);
			if(m_vSubDir.size() > 0) {
				for (vector<CString>::iterator it = m_vSubDir.begin(); it != m_vSubDir.end(); ++it) {
					CString sPath = sFile + "/" + (*it);
					if (stat(sPath.c_str(), &st) != 0) {
						return 0;
					}
					iDirSize = iDirSize + st.st_size;
				}
			}
			return iDirSize;
		}

		if (stat(sFile.c_str(), &st) != 0) {
			return 0;
		}

		return st.st_size;
	}	
};

template <>
void TModInfo<CWebLog>(CModInfo& Info) {
	Info.SetWikiPage("weblog");
}

GLOBALMODULEDEFS(CWebLog, t_s("Allows viewing of log files created by the log module from the webadmin."))
