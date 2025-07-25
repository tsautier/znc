/*
 * Copyright (C) 2004-2025 ZNC, see the NOTICE file for details.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <znc/Listener.h>
#include <znc/Config.h>
#include <znc/znc.h>

CListener::~CListener() {
    if (m_pListener) CZNC::Get().GetManager().DelSockByAddr(m_pListener);
}

CConfig CListener::ToConfig() const {
    CConfig listenerConfig;

    listenerConfig.AddKeyValuePair("URIPrefix", GetURIPrefix() + "/");

    listenerConfig.AddKeyValuePair("SSL", CString(IsSSL()));

    listenerConfig.AddKeyValuePair(
        "AllowIRC",
        CString(GetAcceptType() != CListener::ACCEPT_HTTP));
    listenerConfig.AddKeyValuePair(
        "AllowWeb",
        CString(GetAcceptType() != CListener::ACCEPT_IRC));

    return listenerConfig;
}

void CListener::SetupSSL() const {
#ifdef HAVE_LIBSSL
    if (IsSSL()) {
        m_pListener->SetSSL(true);
        m_pListener->SetPemLocation(CZNC::Get().GetPemLocation());
        m_pListener->SetKeyLocation(CZNC::Get().GetKeyLocation());
        m_pListener->SetDHParamLocation(CZNC::Get().GetDHParamLocation());
    }
#endif
}

CTCPListener::~CTCPListener() {
}

bool CTCPListener::Listen() {
    if (!m_uPort || m_pListener) {
        errno = EINVAL;
        return false;
    }

    m_pListener = new CRealListener(*this);
    SetupSSL();

    // If e.g. getaddrinfo() fails, the following might not set errno.
    // Make sure there is a consistent error message, not something random
    // which might even be "Error: Success".
    errno = EINVAL;
    return CZNC::Get().GetManager().ListenHost(m_uPort, "_LISTENER",
                                               m_sBindHost, IsSSL(), SOMAXCONN,
                                               m_pListener, 0, m_eAddr);
}

CConfig CTCPListener::ToConfig() const {
    CConfig listenerConfig = CListener::ToConfig();

    listenerConfig.AddKeyValuePair("Host", GetBindHost());
    listenerConfig.AddKeyValuePair("Port", CString(GetPort()));

    listenerConfig.AddKeyValuePair(
        "IPv4", CString(GetAddrType() != ADDR_IPV6ONLY));
    listenerConfig.AddKeyValuePair(
        "IPv6", CString(GetAddrType() != ADDR_IPV4ONLY));

    return listenerConfig;
}

CUnixListener::CUnixListener(const CString& sPath, const CString& sURIPrefix,
                             bool bSSL, EAcceptType eAccept,
                             const CString& sGid, const CString& sMode)
    : CListener(sURIPrefix, bSSL, eAccept),
      m_sPath(sPath),
      m_sGid(sGid),
      m_iMode(-1) {
    if (!sMode.empty()) {
        std::istringstream s(sMode);
        s >> std::oct >> m_iMode;
    }
}

CUnixListener::~CUnixListener() {}

bool CUnixListener::Listen() {
    if (m_pListener) {
        errno = EINVAL;
        return false;
    }

    m_pListener = new CRealListener(*this);
    SetupSSL();

    if (!CZNC::Get().GetManager().ListenUnix("UNIX_LISTENER", m_sPath,
                                             m_pListener))
        return false;

    if (!m_sGid.empty()) {
        bool bSuccess = [&]() -> bool {
            std::vector<char> buffer(100);
            group gr{};
            group* result;
        retrysize:
            int err = getgrnam_r(m_sGid.c_str(), &gr, buffer.data(),
                                 buffer.size(), &result);
            switch (err) {
                case ERANGE: {
                    if (buffer.size() > 10000) {
                        DEBUG("Can't get gid due to memory size");
                        return false;
                    }
                    buffer.resize(buffer.size() + 100);
                    goto retrysize;
                }
                case 0: {
                    if (!result) {
                        DEBUG("Group not found");
                        return false;
                    }
                    if (chown(m_sPath.c_str(), -1, result->gr_gid)) {
                        char* e = strerror(errno);
                        DEBUG("Can't chmod: " << e);
                        return false;
                    }
                    break;
                }
                default: {
                    char* e = strerror(err);
                    DEBUG("Error while getting gid " << e);
                    return false;
                }
            }
            return true;
        }();
        if (!bSuccess) {
            m_pListener->Close();
            return false;
        }
    }

    if (m_iMode != -1) {
        if (chmod(m_sPath.c_str(), m_iMode)) {
            char* e = strerror(errno);
            DEBUG("Error while chmod " << e);
            m_pListener->Close();
            return false;
        }
    }

    return true;
}

CConfig CUnixListener::ToConfig() const {
    CConfig listenerConfig = CListener::ToConfig();

    listenerConfig.AddKeyValuePair("Path", GetPath());
    if (!m_sGid.empty()) listenerConfig.AddKeyValuePair("Group", m_sGid);
    if (m_iMode != -1) {
        listenerConfig.AddKeyValuePair("Mode", GetMode());
    }

    return listenerConfig;
}

CString CUnixListener::GetMode() const {
    std::ostringstream s;
    if (m_iMode != -1) {
        s << std::oct << m_iMode;
    }
    return s.str();
}

void CListener::ResetRealListener() { m_pListener = nullptr; }

CRealListener::~CRealListener() { m_Listener.ResetRealListener(); }

bool CRealListener::ConnectionFrom(const CString& sHost, unsigned short uPort) {
    bool bHostAllowed = CZNC::Get().IsHostAllowed(sHost);
    DEBUG(GetSockName() << " == ConnectionFrom(" << sHost << ", " << uPort
                        << ") [" << (bHostAllowed ? "Allowed" : "Not allowed")
                        << "]");
    return bHostAllowed;
}

Csock* CRealListener::GetSockObj(const CString& sHost, unsigned short uPort) {
    CIncomingConnection* pClient = new CIncomingConnection(
        sHost, uPort, m_Listener.GetAcceptType(), m_Listener.GetURIPrefix());
    if (CZNC::Get().AllowConnectionFrom(sHost)) {
        GLOBALMODULECALL(OnClientConnect(pClient, sHost, uPort), NOTHING);
    } else {
        pClient->Write(
            ":irc.znc.in 464 unknown-nick :Too many anonymous connections from "
            "your IP\r\n");
        pClient->Close(Csock::CLT_AFTERWRITE);
        GLOBALMODULECALL(OnFailedLogin("", sHost), NOTHING);
    }
    return pClient;
}

void CRealListener::SockError(int iErrno, const CString& sDescription) {
    DEBUG(GetSockName() << " == SockError(" << sDescription << ", "
                        << strerror(iErrno) << ")");
    if (iErrno == EMFILE) {
        // We have too many open fds, let's close this listening port to be able
        // to continue
        // to work, next rehash will (try to) reopen it.
        CZNC::Get().Broadcast(
            "We hit the FD limit, closing listening socket on [" +
            GetLocalIP() + " : " + CString(GetLocalPort()) + "]");
        CZNC::Get().Broadcast(
            "An admin has to rehash to reopen the listening port");
        Close();
    }
}

CIncomingConnection::CIncomingConnection(const CString& sHostname,
                                         unsigned short uPort,
                                         CListener::EAcceptType eAcceptType,
                                         const CString& sURIPrefix)
    : CZNCSock(sHostname, uPort),
      m_eAcceptType(eAcceptType),
      m_sURIPrefix(sURIPrefix) {
    // The socket will time out in 120 secs, no matter what.
    // This has to be fixed up later, if desired.
    SetTimeout(120, 0);

    SetEncoding("UTF-8");
    EnableReadLine();
}

void CIncomingConnection::ReachedMaxBuffer() {
    if (GetCloseType() != CLT_DONT) return;  // Already closing

    // We don't actually SetMaxBufferThreshold() because that would be
    // inherited by sockets after SwapSockByAddr().
    if (GetInternalReadBuffer().length() <= 4096) return;

    // We should never get here with legitimate requests :/
    Close();
}

void CIncomingConnection::ReadLine(const CString& sLine) {
    bool bIsHTTP = (sLine.WildCmp("GET * HTTP/1.?\r\n") ||
                    sLine.WildCmp("POST * HTTP/1.?\r\n"));
    bool bAcceptHTTP = (m_eAcceptType == CListener::ACCEPT_ALL) ||
                       (m_eAcceptType == CListener::ACCEPT_HTTP);
    bool bAcceptIRC = (m_eAcceptType == CListener::ACCEPT_ALL) ||
                      (m_eAcceptType == CListener::ACCEPT_IRC);
    Csock* pSock = nullptr;

    if (!bIsHTTP) {
        // Let's assume it's an IRC connection

        if (!bAcceptIRC) {
            Write("ERROR :We don't take kindly to your types around here!\r\n");
            Close(CLT_AFTERWRITE);

            DEBUG("Refused IRC connection to non IRC port");
            return;
        }

        pSock = new CClient();
        CZNC::Get().GetManager().SwapSockByAddr(pSock, this);

        // And don't forget to give it some sane name / timeout
        pSock->SetSockName("USR::???");
    } else {
        // This is a HTTP request, let the webmods handle it

        if (!bAcceptHTTP) {
            Write(
                "HTTP/1.0 403 Access Denied\r\n\r\nWeb Access is not "
                "enabled.\r\n");
            Close(CLT_AFTERWRITE);

            DEBUG("Refused HTTP connection to non HTTP port");
            return;
        }

        pSock = new CWebSock(m_sURIPrefix);
        CZNC::Get().GetManager().SwapSockByAddr(pSock, this);

        // And don't forget to give it some sane name / timeout
        pSock->SetSockName("WebMod::Client");
    }

    // TODO can we somehow get rid of this?
    pSock->ReadLine(sLine);
    pSock->PushBuff("", 0, true);
}
