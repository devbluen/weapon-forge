// SA-MP network access: the server hands plugins a getter for its RakServer
// (ppData[PLUGIN_DATA_RAKSERVER]). Its Receive() virtual method is wrapped so
// incoming sync packets can be patched before the server reads them.
// Method indices and the Packet layout are the ones used by Pawn.RakNet.
#include "net_samp.hpp"

#include "core.hpp"

#include <cstddef>
#include <cstring>

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <sys/mman.h>
	#include <unistd.h>
#endif

namespace net_samp
{
	namespace
	{
		constexpr int PLUGIN_DATA_RAKSERVER = 0xE2;

#if defined(_WIN32)
		constexpr int kReceiveIndex = 10;
#else
		constexpr int kReceiveIndex = 11;
#endif

#pragma pack(push, 1)
		struct Packet
		{
			uint16_t player_index;
			uint32_t binary_address;
			uint16_t port;
			uint32_t length;
			uint32_t bit_size;
			unsigned char *data;
			bool delete_data;
		};
#pragma pack(pop)

		static_assert(offsetof(Packet, data) == 16, "RakNet Packet layout");

		using GetRakServer = void *(*)();
		GetRakServer g_get_rakserver = nullptr;
		void **g_vtable_slot = nullptr;
		void *g_original_receive = nullptr;
		bool g_installed = false;
		int g_attempts = 0;

		void Process(Packet *packet)
		{
			if (!packet || !packet->data || packet->length < 1)
			{
				return;
			}

			switch (packet->data[0])
			{
			case 200: // vehicle sync
			case 203: // aim sync
			case 207: // on-foot sync
			case 211: // passenger sync
				wf::ProcessIncomingPacket(packet->player_index, packet->data, packet->length);
				break;
			}
		}

#if defined(_WIN32)
		using ReceiveFn = Packet *(__thiscall *)(void *);

		Packet *__fastcall ReceiveHook(void *self, void * /* edx */)
		{
			Packet *packet = reinterpret_cast<ReceiveFn>(g_original_receive)(self);
			Process(packet);
			return packet;
		}
#else
		using ReceiveFn = Packet *(*)(void *);

		Packet *ReceiveHook(void *self)
		{
			Packet *packet = reinterpret_cast<ReceiveFn>(g_original_receive)(self);
			Process(packet);
			return packet;
		}
#endif

		bool WriteSlot(void **slot, void *value)
		{
#if defined(_WIN32)
			DWORD old = 0;

			if (!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &old))
			{
				return false;
			}

			*slot = value;
			VirtualProtect(slot, sizeof(void *), old, &old);
			return true;
#else
			long page = sysconf(_SC_PAGESIZE);
			uintptr_t start = reinterpret_cast<uintptr_t>(slot) & ~static_cast<uintptr_t>(page - 1);

			if (mprotect(reinterpret_cast<void *>(start), static_cast<size_t>(page) * 2, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
			{
				return false;
			}

			*slot = value;
			return true;
#endif
		}
	}

	void Init(void **ppData)
	{
		g_get_rakserver = reinterpret_cast<GetRakServer>(ppData[PLUGIN_DATA_RAKSERVER]);
	}

	void TryInstall()
	{
		// The RakServer only exists after the server initialised the network
		if (g_installed || !g_get_rakserver || g_attempts > 1000)
		{
			return;
		}

		++g_attempts;

		void *rakserver = g_get_rakserver();

		if (!rakserver)
		{
			return;
		}

		void **vtable = *static_cast<void ***>(rakserver);

		if (!vtable || !vtable[kReceiveIndex])
		{
			wf::Log("WARN: RakServer not recognised, sync features disabled");
			g_attempts = 1001;
			return;
		}

		g_vtable_slot = &vtable[kReceiveIndex];
		g_original_receive = *g_vtable_slot;

		if (!WriteSlot(g_vtable_slot, reinterpret_cast<void *>(&ReceiveHook)))
		{
			wf::Log("WARN: could not hook RakServer::Receive, sync features disabled");
			g_attempts = 1001;
			return;
		}

		g_installed = true;
		wf::g_raknet = true;
		wf::Log("Network hook installed (SA-MP)");
	}

	void Remove()
	{
		if (g_installed && g_vtable_slot)
		{
			WriteSlot(g_vtable_slot, g_original_receive);
		}

		g_installed = false;
	}
}
