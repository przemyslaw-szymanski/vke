#include "Platform/CWindow.h"

#if VKE_LINUX
#include <unistd.h>
#include <dlfcn.h>
#include <xcb/xcb.h>
//#include <xcb/xcb_image.h>

#include "Memory/Memory.h"
#include "Utils/CLogger.h"

namespace VKE
{
    namespace Platform
    {
        struct SWindowInternal
        {
            struct
            {
                xcb_connection_t    *pConnection = nullptr;
                const xcb_setup_t   *pSetup = nullptr;
                uint32_t            windowId = 0;
                uint32_t            visualId = 0;
            } XCB;

        };

        CWindow::CWindow() { }
        CWindow::~CWindow() { }

        void CWindow::Destroy()
        {
            Memory::DestroyObject(&HeapAllocator, &m_pInternal);
        }

        Result CWindow::Create(const SWindowInfo &Info)
        {
            assert(m_pInternal == nullptr);
            m_Info = Info;
            if(VKE_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal)))
            {
                VKE_LOG_ERR("Unable to create internal struct data. Out of Memory.");
                return VKE_ENOMEMORY;
            }

            auto& XCB = m_pInternal->XCB;
            int scr = -1;
            m_pInternal->pXcbConnection = xcb_connect(nullptr, &scr);
            if(!XCB.pConnection)
            {
                VKE_LOG_ERR("Unable to create xcb connection.");
                return VKE_FAIL;
            }

            XCB.pSetup = xcb_get_setup(XCB.pConnection);
            xcb_screen_iterator_t itr = xcb_setup_roots_iterator(XCB.pSetup);

            while(scr-->0)
            {
                xcb_screen_next(&itr);
            }
            xcb_screen_t* pXcbScreen = itr.data;
            XCB.windowId = xcb_generate_id(XCB.pConnection);

            uint32_t aValues[2];
            aValues[0] = pXcbScreen->black_pixel;
            aValues[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

            XCB.visualId = pXcbScreen->root_visual;

            xcb_create_window(XCB.pConnection, XCB_COPY_FROM_PARENT, XCB.windowId, pXcbScreen->root,
                              0, 0, m_Info.Size.width, m_Info.Size.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                              XCB.visualId, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, aValues);

            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(XCB.pConnection, 1, 12, "WM_PROTOCOLS");
            xcb_intern_atom_reply_t* pReply = xcb_intern_atom_reply(XCB.pConnection, cookie, 0);
            xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(XCB.pConnection, 0, 16, "WM_DELETE_WINDOW");
            auto deleteWindow = xcb_intern_atom_reply(XCB.pConnection, cookie2, 0);
        }

        void CWindow::Update() { }
    } // Platform
} // VKE
#endif // VKE_LINUX