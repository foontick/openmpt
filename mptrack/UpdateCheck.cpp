/*
 * UpdateCheck.cpp
 * ---------------
 * Purpose: Class for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "UpdateCheck.h"
#include "BuildVariants.h"
#include "../common/version.h"
#include "../common/misc_util.h"
#include "Mptrack.h"
#include "TrackerSettings.h"
// Setup dialog stuff
#include "Mainfrm.h"
#include "../common/mptThread.h"
#include "HTTP.h"


OPENMPT_NAMESPACE_BEGIN

// Update notification dialog
class UpdateDialog : public CDialog
{
protected:
	const CString &m_releaseVersion;
	const CString &m_releaseDate;
	const CString &m_releaseURL;
	CFont m_boldFont;
public:
	bool ignoreVersion;

public:
	UpdateDialog(const CString &releaseVersion, const CString &releaseDate, const CString &releaseURL)
		: CDialog(IDD_UPDATE)
		, m_releaseVersion(releaseVersion)
		, m_releaseDate(releaseDate)
		, m_releaseURL(releaseURL)
		, ignoreVersion(false)
	{ }

	BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();

		CFont *font = GetDlgItem(IDC_VERSION2)->GetFont();
		LOGFONT lf;
		font->GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
		m_boldFont.CreateFontIndirect(&lf);
		GetDlgItem(IDC_VERSION2)->SetFont(&m_boldFont);

		SetDlgItemText(IDC_VERSION1, mpt::cfmt::val(Version::Current()));
		SetDlgItemText(IDC_VERSION2, m_releaseVersion);
		SetDlgItemText(IDC_DATE, m_releaseDate);
		SetDlgItemText(IDC_SYSLINK1, _T("More information about this build:\n<a href=\"") + m_releaseURL + _T("\">") + m_releaseURL + _T("</a>"));
		CheckDlgButton(IDC_CHECK1, (TrackerSettings::Instance().UpdateIgnoreVersion == m_releaseVersion) ? BST_CHECKED : BST_UNCHECKED);
		return FALSE;
	}

	void OnClose()
	{
		m_boldFont.DeleteObject();
		CDialog::OnClose();
		TrackerSettings::Instance().UpdateIgnoreVersion = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED ? m_releaseVersion : CString(_T(""));
	}

	void OnClickURL(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
	{
		CTrackApp::OpenURL(m_releaseURL);
	}

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(UpdateDialog, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK1, &UpdateDialog::OnClickURL)
END_MESSAGE_MAP()




mpt::ustring CUpdateCheck::GetDefaultUpdateURL()
{
	return MPT_USTRING("https://update.openmpt.org/check/$VERSION/$GUID");
}


std::atomic<int32> CUpdateCheck::s_InstanceCount(0);


int32 CUpdateCheck::GetNumCurrentRunningInstances()
{
	return s_InstanceCount.load();
}


// Start update check
void CUpdateCheck::StartUpdateCheckAsync(bool isAutoUpdate)
{
	if(isAutoUpdate)
	{
		int updateCheckPeriod = TrackerSettings::Instance().UpdateUpdateCheckPeriod;
		if(updateCheckPeriod == 0)
		{
			return;
		}
		// Do we actually need to run the update check right now?
		const time_t now = time(nullptr);
		if(difftime(now, TrackerSettings::Instance().UpdateLastUpdateCheck.Get()) < (double)(updateCheckPeriod * 86400))
		{
			return;
		}

		// Never ran update checks before, so we notify the user of automatic update checks.
		if(TrackerSettings::Instance().UpdateShowUpdateHint)
		{
			TrackerSettings::Instance().UpdateShowUpdateHint = false;
			CString msg;
			msg.Format(_T("OpenMPT would like to check for updates now, proceed?\n\nNote: In the future, OpenMPT will check for updates every %u days. If you do not want this, you can disable update checks in the setup."), TrackerSettings::Instance().UpdateUpdateCheckPeriod.Get());
			if(Reporting::Confirm(msg, _T("OpenMPT Internet Update")) == cnfNo)
			{
				TrackerSettings::Instance().UpdateLastUpdateCheck = mpt::Date::Unix(now);
				return;
			}
		}
	}
	TrackerSettings::Instance().UpdateShowUpdateHint = false;

	int32 expected = 0;
	if(!s_InstanceCount.compare_exchange_strong(expected, 1))
	{
		return;
	}

	CUpdateCheck::Settings settings;
	settings.window = CMainFrame::GetMainFrame();
	settings.msgProgress = MPT_WM_APP_UPDATECHECK_PROGRESS;
	settings.msgSuccess = MPT_WM_APP_UPDATECHECK_SUCCESS;
	settings.msgFailure = MPT_WM_APP_UPDATECHECK_FAILURE;
	settings.autoUpdate = isAutoUpdate;
	settings.updateBaseURL = TrackerSettings::Instance().UpdateUpdateURL;
	settings.sendStatistics = TrackerSettings::Instance().UpdateSendGUID;
	settings.statisticsUUID = TrackerSettings::Instance().VersionInstallGUID;
	settings.suggestDifferentBuilds = TrackerSettings::Instance().UpdateSuggestDifferentBuildVariant;
	std::thread(CUpdateCheck::ThreadFunc(settings)).detach();
}


CUpdateCheck::ThreadFunc::ThreadFunc(const CUpdateCheck::Settings &settings)
	: settings(settings)
{
	return;
}


void CUpdateCheck::ThreadFunc::operator () ()
{
	mpt::SetCurrentThreadPriority(settings.autoUpdate ? mpt::ThreadPriorityLower : mpt::ThreadPriorityNormal);
	CUpdateCheck::CheckForUpdate(settings);
}


// Run update check (independent thread)
CUpdateCheck::Result CUpdateCheck::SearchUpdate(const CUpdateCheck::Settings &settings)
{
	
	HTTP::InternetSession internet(Version::Current().GetOpenMPTVersionString());

	mpt::ustring updateURL = settings.updateBaseURL;
	if(updateURL.empty())
	{
		updateURL = GetDefaultUpdateURL();
	}
	if(updateURL.find(MPT_USTRING("://")) == mpt::ustring::npos)
	{
		updateURL = MPT_USTRING("https://") + updateURL;
	}

	// Build update URL
	updateURL = mpt::String::Replace(updateURL, MPT_USTRING("$VERSION"), mpt::uformat(MPT_USTRING("%1-%2-%3"))
		( Version::Current()
		, BuildVariants().GuessCurrentBuildName()
		, settings.sendStatistics ? mpt::Windows::Version::Current().GetNameShort() : MPT_USTRING("unknown")
		));
	updateURL = mpt::String::Replace(updateURL, MPT_USTRING("$GUID"), settings.sendStatistics ? mpt::ufmt::val(settings.statisticsUUID) : MPT_USTRING("anonymous"));

	// Establish a connection.
	HTTP::Request request;
	request.SetURI(ParseURI(updateURL));
	request.method = HTTP::Method::Get;
	request.flags = HTTP::NoCache;

	HTTP::Result resultHTTP = internet(request.InsecureTLSDowngradeWindowsXP());

	// Retrieve HTTP status code.
	if(resultHTTP.Status >= 400)
	{
		throw CUpdateCheck::Error(mpt::cformat(_T("Version information could not be found on the server (HTTP status code %1). Maybe your version of OpenMPT is too old!"))(resultHTTP.Status));
	}

	// Now, evaluate the downloaded data.
	CUpdateCheck::Result result;
	result.UpdateAvailable = false;
	result.CheckTime = time(nullptr);
	CString resultData = mpt::ToCString(mpt::CharsetUTF8, resultHTTP.Data);
	if(resultData.CompareNoCase(_T("noupdate")) != 0)
	{
		CString token;
		int parseStep = 0, parsePos = 0;
		while((token = resultData.Tokenize(_T("\n"), parsePos)) != "")
		{
			token.Trim();
			switch(parseStep++)
			{
			case 0:
				if(token.CompareNoCase(_T("update")) != 0)
				{
					throw CUpdateCheck::Error(_T("Could not understand server response. Maybe your version of OpenMPT is too old!"));
				}
				break;
			case 1:
				result.Version = token;
				break;
			case 2:
				result.Date = token;
				break;
			case 3:
				result.URL = token;
				break;
			}
		}
		if(parseStep < 4)
		{
			throw CUpdateCheck::Error(_T("Could not understand server response. Maybe your version of OpenMPT is too old!"));
		}
		result.UpdateAvailable = true;
	}
	return result;
}


void CUpdateCheck::CheckForUpdate(const CUpdateCheck::Settings &settings)
{
	// íncremented before starting the thread
	MPT_ASSERT(s_InstanceCount.load() >= 1);
	CUpdateCheck::Result result;
	settings.window->SendMessage(settings.msgProgress, settings.autoUpdate ? 1 : 0, s_InstanceCount.load());
	try
	{
		try
		{
			result = SearchUpdate(settings);
		} catch(const bad_uri &e)
		{
			throw CUpdateCheck::Error(mpt::cformat(_T("Error parsing update URL: %1"))(mpt::get_exception_text<CString>(e)));
		} catch(const HTTP::exception &e)
		{
			throw CUpdateCheck::Error(CString(_T("HTTP error: ")) + mpt::ToCString(e.GetMessage()));
		}
	} catch(const CUpdateCheck::Error &e)
	{
		settings.window->SendMessage(settings.msgFailure, settings.autoUpdate ? 1 : 0, reinterpret_cast<LPARAM>(&e));
		s_InstanceCount.fetch_sub(1);
		MPT_ASSERT(s_InstanceCount.load() >= 0);
		return;
	}
	settings.window->SendMessage(settings.msgSuccess, settings.autoUpdate ? 1 : 0, reinterpret_cast<LPARAM>(&result));
	s_InstanceCount.fetch_sub(1);
	MPT_ASSERT(s_InstanceCount.load() >= 0);
}


CUpdateCheck::Result CUpdateCheck::ResultFromMessage(WPARAM /*wparam*/ , LPARAM lparam)
{
	const CUpdateCheck::Result &result = *reinterpret_cast<CUpdateCheck::Result*>(lparam);
	return result;
}


CUpdateCheck::Error CUpdateCheck::ErrorFromMessage(WPARAM /*wparam*/ , LPARAM lparam)
{
	const CUpdateCheck::Error &error = *reinterpret_cast<CUpdateCheck::Error*>(lparam);
	return error;
}


void CUpdateCheck::ShowSuccessGUI(WPARAM wparam, LPARAM lparam)
{
	const CUpdateCheck::Result &result = *reinterpret_cast<CUpdateCheck::Result*>(lparam);
	bool autoUpdate = wparam != 0;
	if(result.UpdateAvailable && (!autoUpdate || result.Version != TrackerSettings::Instance().UpdateIgnoreVersion))
	{
		UpdateDialog dlg(result.Version, result.Date, result.URL);
		if(dlg.DoModal() == IDOK)
		{
			CTrackApp::OpenURL(result.URL);
		}
	} else if(!result.UpdateAvailable && !autoUpdate)
	{
		Reporting::Information(MPT_USTRING("You already have the latest version of OpenMPT installed."), MPT_USTRING("OpenMPT Internet Update"));
	}
}


void CUpdateCheck::ShowFailureGUI(WPARAM wparam, LPARAM lparam)
{
	const CUpdateCheck::Error &error = *reinterpret_cast<CUpdateCheck::Error*>(lparam);
	bool autoUpdate = wparam != 0;
	if(!autoUpdate)
	{
		Reporting::Error(mpt::get_exception_text<mpt::ustring>(error), MPT_USTRING("OpenMPT Internet Update Error"));
	}
}


CUpdateCheck::Error::Error(CString errorMessage)
	: std::runtime_error(mpt::ToCharset(mpt::CharsetUTF8, errorMessage))
{
	return;
}


CUpdateCheck::Error::Error(CString errorMessage, DWORD errorCode)
	: std::runtime_error(mpt::ToCharset(mpt::CharsetUTF8, FormatErrorCode(errorMessage, errorCode)))
{
	return;
}


CString CUpdateCheck::Error::FormatErrorCode(CString errorMessage, DWORD errorCode)
{
	void *lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		GetModuleHandle(TEXT("wininet.dll")), errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	errorMessage.Append((LPTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return errorMessage;
}


/////////////////////////////////////////////////////////////
// CUpdateSetupDlg

BEGIN_MESSAGE_MAP(CUpdateSetupDlg, CPropertyPage)
	ON_COMMAND(IDC_BUTTON1,			&CUpdateSetupDlg::OnCheckNow)
	ON_COMMAND(IDC_BUTTON2,			&CUpdateSetupDlg::OnResetURL)
	ON_COMMAND(IDC_RADIO1,			&CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,			&CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,			&CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO4,			&CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,			&CUpdateSetupDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT1,			&CUpdateSetupDlg::OnSettingsChanged)
END_MESSAGE_MAP()


CUpdateSetupDlg::CUpdateSetupDlg()
	: CPropertyPage(IDD_OPTIONS_UPDATE)
	, m_SettingChangedNotifyGuard(theApp.GetSettings(), TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath())
{
	return;
}


BOOL CUpdateSetupDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	int radioID = 0;
	int periodDays = TrackerSettings::Instance().UpdateUpdateCheckPeriod;
	if(periodDays >= 30)
	{
		radioID = IDC_RADIO4;
	} else if(periodDays >= 7)
	{
		radioID = IDC_RADIO3;
	} else if(periodDays >= 1)
	{
		radioID = IDC_RADIO2;
	} else
	{
		radioID = IDC_RADIO1;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO4, radioID);
	CheckDlgButton(IDC_CHECK1, TrackerSettings::Instance().UpdateSendGUID ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(IDC_EDIT1, mpt::ToCString(TrackerSettings::Instance().UpdateUpdateURL.Get()));

	m_SettingChangedNotifyGuard.Register(this);
	SettingChanged(TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath());

	return TRUE;
}


void CUpdateSetupDlg::SettingChanged(const SettingPath &changedPath)
{
	if(changedPath == TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath())
	{
		CString updateText;
		const time_t t = TrackerSettings::Instance().UpdateLastUpdateCheck.Get();
		if(t > 0)
		{
			const tm* const lastUpdate = localtime(&t);
			if(lastUpdate != nullptr)
			{
				updateText.Format(_T("The last successful update check was run on %04d-%02d-%02d, %02d:%02d."), lastUpdate->tm_year + 1900, lastUpdate->tm_mon + 1, lastUpdate->tm_mday, lastUpdate->tm_hour, lastUpdate->tm_min);
			}
		}
		updateText += _T("\r\n");
		if(TrackerSettings::Instance().UpdateSuggestDifferentBuildVariant)
		{
			updateText += theApp.SuggestModernBuildText();
		}
		SetDlgItemText(IDC_LASTUPDATE, updateText);
	}
}


void CUpdateSetupDlg::OnOK()
{
	int updateCheckPeriod = TrackerSettings::Instance().UpdateUpdateCheckPeriod;
	if(IsDlgButtonChecked(IDC_RADIO1)) updateCheckPeriod = 0;
	if(IsDlgButtonChecked(IDC_RADIO2)) updateCheckPeriod = 1;
	if(IsDlgButtonChecked(IDC_RADIO3)) updateCheckPeriod = 7;
	if(IsDlgButtonChecked(IDC_RADIO4)) updateCheckPeriod = 31;

	CString updateURL;
	GetDlgItemText(IDC_EDIT1, updateURL);

	TrackerSettings::Instance().UpdateUpdateCheckPeriod = updateCheckPeriod;
	TrackerSettings::Instance().UpdateUpdateURL = mpt::ToUnicode(updateURL);
	TrackerSettings::Instance().UpdateSendGUID = (IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	
	CPropertyPage::OnOK();
}


BOOL CUpdateSetupDlg::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_UPDATE;
	return CPropertyPage::OnSetActive();
}


void CUpdateSetupDlg::OnCheckNow()
{
	CMainFrame::GetMainFrame()->PostMessage(WM_COMMAND, ID_INTERNETUPDATE);
}


void CUpdateSetupDlg::OnResetURL()
{
	SetDlgItemText(IDC_EDIT1, mpt::ToCString(CUpdateCheck::GetDefaultUpdateURL()));
}


OPENMPT_NAMESPACE_END
