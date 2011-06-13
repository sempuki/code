#include <stdio.h>
#include <util/ChannelManager.h>
#include <Platform/Platform.h>

#include <cell/sysmodule.h>
#include <netex/net.h>
#include <netex/libnetctl.h>
#include <sys/sys_time.h>
#include <sdk_version.h>
#include <np.h>
#include <np/basic.h>

#include <Tasks/OnlineTasks.h>
#include <Peer2Peer/Peer2Peer.h>

namespace AK
{
	void * AllocHook( size_t bytes )
	{
		void* pmem = XP_ALLOCATE_TAG(bytes, "Wwise");
		ASSERT(pmem != NULL);
		return pmem;
	}
	
    void FreeHook( void * memory )
	{
		XP_FREE(memory);
	}
}

// NOTE: the API documents are inconsistent in their integer type treatment:
// assuming integers are 4-byte. This is perhaps not an unfair assumption 
// given the stable nature of the PS3 hardware, but may not always be true.
// We just use unadorned (unsigned) int.

namespace Network
{
    namespace PS3
    {
        namespace Product
        {
            static const SceNpCommunicationId Id = 
            {
                {'N', 'P', 'W', 'R', '0', '2', '1', '6', '3'},
                '\0',
                0,
                0
            };

            static const SceNpCommunicationPassphrase Passphrase = 
            {
                {
                    0x07,0xe3,0x12,0xdf,0x9d,0x96,0xf2,0xf5,
                    0x29,0xe9,0xe4,0x72,0x1a,0x55,0x40,0x18,
                    0xba,0xfe,0x59,0x5f,0x04,0x99,0x0f,0x98,
                    0x60,0x8e,0x7b,0xbe,0xad,0xf6,0x24,0xd5,
                    0x3c,0x7b,0xd6,0x03,0xc1,0xb2,0x64,0xb6,
                    0x03,0x5c,0xe3,0xaa,0x85,0xa2,0x67,0x4e,
                    0xbc,0x66,0xbe,0xda,0xa5,0xda,0xe1,0x88,
                    0x78,0x2b,0x0f,0xef,0xcd,0xd5,0x4e,0x95,
                    0x38,0x14,0x1b,0xf6,0xbc,0x5e,0x9b,0xf5,
                    0x9c,0xf0,0xf0,0xe6,0x5f,0x1b,0x68,0x96,
                    0xbe,0xa7,0xa9,0xf7,0xe4,0x3f,0xd3,0xfc,
                    0xac,0x7d,0x3d,0xb2,0xb5,0x59,0x33,0x39,
                    0xdc,0x87,0xaa,0x60,0xaa,0x9c,0xb6,0x92,
                    0x01,0x18,0x88,0xdd,0xa2,0x02,0x80,0x37,
                    0x5c,0x0d,0xd2,0x04,0x53,0x6f,0xc6,0x04,
                    0x2b,0x5d,0x75,0xc2,0x7a,0xec,0xb0,0xb1
                }
            };

            static const SceNpCommunicationSignature Signature = 
            {
                {
                    0xb9,0xdd,0xe1,0x3b,0x01,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0xc6,0x23,0x0d,0x2a,
                    0x26,0x94,0xa7,0xcf,0xcd,0x54,0xb2,0x30,
                    0x12,0x97,0x5d,0x6a,0x76,0xfb,0xbf,0x42,
                    0xa7,0x5d,0x33,0x5c,0x19,0x92,0x59,0x9c,
                    0x2c,0x6f,0x2b,0x3b,0x7e,0xa7,0xe1,0xcc,
                    0x85,0x92,0x60,0xdc,0x55,0x84,0x7c,0x24,
                    0x41,0x40,0xf4,0x44,0x2c,0x81,0xe5,0xc1,
                    0x35,0xaf,0x9e,0xb8,0x01,0x53,0x29,0x3d,
                    0xff,0x93,0xe1,0x15,0xdc,0x11,0x16,0x79,
                    0xa2,0xf2,0x37,0x6d,0x89,0x1a,0xa6,0xae,
                    0xfb,0xe3,0xb4,0xbf,0x0e,0x17,0xc2,0xfa,
                    0x1d,0x9e,0xc7,0xd0,0x32,0x70,0x97,0x47,
                    0x9e,0xb0,0xce,0x4e,0x96,0x6a,0x38,0x37,
                    0x5e,0x5e,0x5d,0x2e,0xae,0x93,0x33,0xfd,
                    0x7f,0x6a,0x3d,0xf3,0x74,0xc0,0x76,0xed,
                    0xf1,0x89,0x8e,0x80,0x36,0x98,0x84,0xca,
                    0xc3,0xc7,0x61,0xc1,0x29,0xf6,0xde,0xb8,
                    0x15,0x4e,0x50,0xe0,0xc0,0x78,0x6c,0x74,
                    0x55,0x46,0xfa,0xd1,0x00,0xa3,0xcc,0xe9
                }
            };
        };

        struct User
        {
            User()
            {
                memset(&info, 0, sizeof(info));
                memset(&onlineId, 0, sizeof(onlineId));
                memset(&about, 0, sizeof(about));
                memset(&languages, 0, sizeof(languages));
                memset(&country, 0, sizeof(country));
                memset(&avatar, 0, sizeof(avatar));
            }

            User(const User &u)
            {
                memcpy(&info, &u.info, sizeof(info));
                memcpy(&onlineId, &u.onlineId, sizeof(onlineId));
                memcpy(&about, &u.about, sizeof(about));
                memcpy(&languages, &u.languages, sizeof(languages));
                memcpy(&country, &u.country, sizeof(country));
                memcpy(&avatar, &u.avatar, sizeof(avatar));
            }

            SceNpUserInfo       info;
            SceNpOnlineId       onlineId;
            SceNpAboutMe        about;
            SceNpMyLanguages    languages;
            SceNpCountryCode    country;
            SceNpAvatarImage    avatar;
        };

        struct Ticket
        {
            Ticket(const char *s) : service(s), ready(false)
            {
                memset(&version, 0, sizeof(version));
            }

            Ticket(const Ticket &t) : service(t.service), ready(t.ready)
            {
                memcpy(&version, &t.version, sizeof(version));
            }

            const char*         service;
            SceNpTicketVersion  version;
            bool                ready;

            struct Param
            {
                Param(int p) : id(p)
                {
                    memset(&param, 0, sizeof(param));

                    int res = sceNpManagerGetTicketParam(id, &param);
                    ASSERTF(res == 0, "[Network::PS3::Ticket::Param] Could not get ticket parameter. (0x%x)\n", res);
                }

                Param(const Param &p) : id(p.id)
                {
                    memcpy(&param, &p.param, sizeof(param));
                }

                int                 id;
                SceNpTicketParam    param;
            };

            struct Entitlement
            {
                Entitlement(const char *e, size_t n) 
                    : entitlement(e), number(n)
                {}

                Entitlement(const Entitlement &e)
                    : entitlement(e.entitlement), number(e.number)
                {}

                const char* entitlement;
                size_t      number;
            };

            struct Cookie
            {
                Cookie(const char *d, size_t s) : data(d), size(s) {}
                Cookie(const Cookie &c) : data(c.data), size(c.size) {}

                const void* data;
                size_t      size;
            };
        };

        class System
        {
            public:
                System() {}
                ~System() {}

                enum SysutilSlotType { SYSUTIL_SLOT = 3 };

                void Initialize()
                {
                    if (cellSysmoduleInitialize()							!= CELL_OK ||
                        cellSysmoduleLoadModule(CELL_SYSMODULE_NET)			!= CELL_OK ||
                        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP)	!= CELL_OK ||
                        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP2) != CELL_OK)
                        ABORTF("[Network::PS3::System] Could not load CELL_SYSMODULE_NET or CELL_SYSMODULE_SYSUTIL_NP2 or could not initialize PS3 sockets.");

                    int res = sys_net_initialize_network(); // initialize UDP/IP stack
                    ASSERTF(res >= 0, "sys_net_initialize_network() failed. (0x%x)\n", res);

                    res = sys_net_show_ifconfig(); // print ifconfig to TTY
                    ASSERTF(res >= 0, "sys_net_show_ifconfig() failed. (0x%x)\n", res);

                    res = cellNetCtlInit(); // initialize network control
                    ASSERTF(res >= 0,"cellNetCtlInit() failed. (0x%x)\n", res);

                    res = sceNp2Init(SCE_NP_MIN_POOL_SIZE, m_nppool);
                    ASSERTF(res == 0, "[Network::PS3::System] Could not initialize system. (0x%x)\n", res);

                    res = cellSysutilRegisterCallback(SYSUTIL_SLOT, &System::sysutil_cb, 0);
                    ASSERTF(res == 0, "[Network::PS3::System] Could not set sysutil callback. (0x%x)\n", res);
                }

                void Finalize()
                {
                    int res = cellSysutilUnregisterCallback(SYSUTIL_SLOT);
                    ASSERTF(res == 0, "[Network::PS3::System] Could not terminate system. (0x%x)\n", res);

                    res = sceNp2Term();
                    ASSERTF(res == 0, "[Network::PS3::System] Could not terminate system. (0x%x)\n", res);

                    cellNetCtlTerm();

                    res = sys_net_finalize_network();
                    ASSERTF(res == 0, "[Network::PS3::System] Could not finalize system. (0x%x)\n", res);

                    if (cellSysmoduleUnloadModule(CELL_SYSMODULE_NET)			!= CELL_OK ||
                        cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_NP)	!= CELL_OK ||
                        cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_NP2)   != CELL_OK ||
                        cellSysmoduleFinalize()							        != CELL_OK)
                        ABORTF("[Network::PS3::System] Could not load CELL_SYSMODULE_NET or CELL_SYSMODULE_SYSUTIL_NP2 or could not finalize PS3 sockets.");
                }

                void Update()
                {
                    int res = cellSysutilCheckCallback(); // pump system utility events
                    ASSERTF(res == 0, "[Network::PS3::Sytem] Could not update system. (0x%x)\n", res);
                }

            private:
                static void sysutil_cb(uint64_t status, uint64_t param, void *data)
                {
                    //switch (status)
                    //{
                    //    case CELL_SYSUTIL_REQUEST_EXITGAME:
                    //    case CELL_SYSUTIL_DRAWING_BEGIN:
                    //    case CELL_SYSUTIL_DRAWING_END:
                    //    case CELL_SYSUTIL_SYSTEM_MENU_OPEN:
                    //    case CELL_SYSUTIL_SYSTEM_MENU_CLOSE:
                    //    case CELL_SYSUTIL_BGMPLAYBACK_PLAY:
                    //    case CELL_SYSUTIL_BGMPLAYBACK_STOP:
                    //    case CELL_SYSUTIL_NP_INVITATION_SELECTED:
                    //    default:
                    //        break;
                    //}
                }

            private:
                uint8_t m_nppool[SCE_NP_MIN_POOL_SIZE];
        };

        class Manager
        {
            public:
                struct Transaction
                {
                    Transaction(int ctx, const User &u) 
                        : lookupctx(ctx), user(u)
                    {
                        id = sceNpLookupCreateTransactionCtx(lookupctx);
                        ASSERTF(id == 0, "[Network::PS3::Manager::Transaction] Could not create transaction context. (0x%x)\n", id);
                    }

                    ~Transaction()
                    {
                        id = sceNpLookupDestroyTransactionCtx(lookupctx);
                        ASSERTF(id == 0, "[Network::PS3::Manager::Transaction] Could not destroy transaction context. (0x%x)\n", id);
                    }

                    bool Poll()
                    {
                        int res = sceNpLookupPollAsync(id, &result);
                        ASSERTF(id >= 0, "[Network::PS3::Manager::Transaction] Failed to poll transaction. (0x%x)\n", res);
                        return res != 0;
                    }

                    bool Wait()
                    {
                        int res = sceNpLookupWaitAsync(id, &result);
                        ASSERTF(id >= 0, "[Network::PS3::Manager::Transaction] Failed to wait for transaction. (0x%x)\n", res);
                        return res == 0;
                    }

                    bool Abort()
                    {
                        int res = sceNpLookupAbortTransaction(id);
                        ASSERTF(id >= 0, "[Network::PS3::Manager::Transaction] Failed to abort transaction. (0x%x)\n", res);
                        return true;
                    }

                    int         id;
                    int         result;

                    const int   lookupctx;
                    const User& user;
                };

            private:
                struct TicketUpdater
                {
                    Ticket *ticket;
                    TicketUpdater() : ticket(0) {}
                    TicketUpdater(Ticket *t) : ticket(t) {}
                    void operator()(bool status, int size) { ticket->ready = status; }
                };

            public:
                Manager() {}
                ~Manager() {}

                void Initialize()
                {
                    int res = sceNpManagerInit();
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not initialize manager. (0x%x)\n", res);

                    res = sceNpLookupInit();
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not initialize lookup.(0x%x)\n", res);

                    res = sceNpManagerRegisterCallback(&Manager::manager_cb, this);
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not register callback. (0x%x)\n", res);

                    res = TryGetLocalUser(m_local);
                    ASSERTF(res == 1, "[Network::PS3::Manager] Could not get local user. (0x%x)\n", res);

                    res = sceNpLookupCreateTitleCtx(&Product::Id, &m_local.info.userId);
                    ASSERTF(res > 0, "[Network::PS3::Manager] Could not create lookup context. (0x%x)\n", res);

                    m_lookupctx = res;
                }

                void Finalize()
                {
                    int res = sceNpLookupDestroyTitleCtx(m_lookupctx);
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not destroy lookup context. (0x%x)\n", res);

                    res = sceNpManagerUnregisterCallback();
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not unregister callback. (0x%x)\n", res);
                    
                    res = sceNpLookupTerm();
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not finalize lookup. (0x%x)\n", res);

                    res = sceNpManagerTerm();
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not finalize manager. (0x%x)\n", res);
                }

                bool TryGetStatus(int &status)
                {
                    return sceNpManagerGetStatus(&status) == 0;
                }

                bool TryGetLocalUser(User &user)
                {
                    int res = sceNpManagerGetNpId(&user.info.userId);
                    ASSERTF(res >= 0, "[Network::PS3::User] Get NpId failed. (0x%x)\n", res);

                    res = sceNpManagerGetOnlineId(&user.onlineId);
                    ASSERTF(res >= 0, "[Network::PS3::User] Get OnlineId failed. (0x%x)\n", res);

                    res = sceNpManagerGetOnlineName(&user.info.name);
                    ASSERTF(res >= 0, "[Network::PS3::User] Get OnlineName failed. (0x%x)\n", res);

                    return true;
                }

                bool TryGetTicket(Ticket &ticket)
                {
                    // TODO: handle cookies and entitlements
                    int res = sceNpManagerRequestTicket2(&m_local.info.userId, &ticket.version, ticket.service, 0, 0, 0, 0);
                    ASSERTF(res == 0, "[Network::PS3::Manager] Could not request ticket. (0x%x)\n", res);

                    ASSERTF(m_updater.ticket->ready, "[Network::PS3::Manager] Previous request ticket did not complete. (%s)\n", \
                            m_updater.ticket->service);

                    m_updater = TicketUpdater(&ticket);

                    return true;
                }

            private:
                static void manager_cb(int event, int result, void *data)
                {
                    ASSERTF(result >= 0, "[Network::PS3::Manager] Error with ticket request. (0x%x)\n", result);
                        
                    switch (event)
                    {
                        case SCE_NP_MANAGER_EVENT_GOT_TICKET:
                            {
                                Manager *m = static_cast<Manager *>(data);
                                m->m_updater(true, result); // report ticket readiness and size
                            }
                            break;
                    }
                }

            private:
                User            m_local;
                TicketUpdater   m_updater;
                int             m_lookupctx;
        };

        class MatchMaker
        {
            public:
                MatchMaker()
                {
                }

                void Initialize()
                {
                }

                void Finalize()
                {
                }

            private:
        };
    }

    typedef PS3::System System;
    typedef PS3::Manager Manager;
    typedef PS3::MatchMaker MatchMaker;
}

template <typename T>
class Using
{
    public:
        Using(T *o) : obj(o) 
        { 
            obj->Initialize(); 
        }

        ~Using() 
        { 
            obj->Finalize(); 
            delete obj, obj = 0;
        }

        T& operator->() const 
        { 
            return *obj; 
        }

    private:
        T   *obj;
};

int main(int argc, char** argv)
{
    Using<Network::System> system(new Network::System);
    Using<Network::Manager> manager(new Network::Manager);
    Using<Network::MatchMaker> matchmaker(new Network::MatchMaker);

	return 0;
}

