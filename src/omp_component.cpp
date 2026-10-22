// open.mp component entry point. Installed in components/, the same binary
// runs as an open.mp component: it gets the AMX functions from the Pawn
// component (so all the AMX hooks work exactly like on SA-MP) and reads the
// incoming sync packets through open.mp's network events.
#include "core.hpp"
#include "main.hpp"
#include "version.hpp"

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>

#include <cstdarg>

namespace wf_omp
{
	ICore *g_core = nullptr;

	void ComponentLog(const char *format, ...)
	{
		if (!g_core)
		{
			return;
		}

		va_list args;
		va_start(args, format);
		g_core->vprintLn(format, args);
		va_end(args);
	}

	// Leading members of open.mp's NetworkBitStream (RakNet's BitStream layout)
	struct BitStreamView
	{
		int bits_used;
		int bits_allocated;
		int read_offset;
		unsigned char *data;
	};

	class WeaponForgeComponent final : public IComponent,
									   public PawnEventHandler,
									   public CoreEventHandler,
									   public NetworkInEventHandler
	{
	public:
		PROVIDE_UID(0x9b6f1d2a5c3e4f70);

		StringView componentName() const override
		{
			return "weapon-forge";
		}

		SemanticVersion componentVersion() const override
		{
			return SemanticVersion(WF_VERSION_NUMBER / 10000, (WF_VERSION_NUMBER / 100) % 100, WF_VERSION_NUMBER % 100, 0);
		}

		void onLoad(ICore *core) override
		{
			core_ = core;
			g_core = core;
		}

		void onInit(IComponentList *components) override
		{
			pawn_ = components->queryComponent<IPawnComponent>();

			if (!pawn_)
			{
				core_->logLn(LogLevel::Error, "[weapon-forge] The Pawn component is not loaded");
				return;
			}

			auto &exports = pawn_->getAmxFunctions();

			if (!wf::Startup(const_cast<void **>(exports.data()), &ComponentLog, true))
			{
				pawn_ = nullptr;
				return;
			}

			core_->getEventDispatcher().addEventHandler(this);
			pawn_->getEventDispatcher().addEventHandler(this);

			for (INetwork *network : core_->getNetworks())
			{
				network->getInEventDispatcher().addEventHandler(this);
			}

			wf::g_raknet = true;
		}

		void onAmxLoad(IPawnScript &script) override
		{
			wf::AmxLoad(static_cast<AMX *>(script.GetAMX()));
		}

		void onAmxUnload(IPawnScript &script) override
		{
			wf::AmxUnload(static_cast<AMX *>(script.GetAMX()));
		}

		void onTick(Microseconds, TimePoint) override
		{
			wf::ServerTick();
		}

		bool onReceivePacket(IPlayer &peer, int id, NetworkBitStream &bs) override
		{
			switch (id)
			{
			case 200: // vehicle sync
			case 203: // aim sync
			case 207: // on-foot sync
			case 211: // passenger sync
			{
				auto &view = reinterpret_cast<BitStreamView &>(bs);

				if (view.data && view.bits_used >= 8 && view.data[0] == id)
				{
					wf::ProcessIncomingPacket(peer.getID(), view.data, static_cast<size_t>((view.bits_used + 7) / 8));
				}
				break;
			}
			}

			return true;
		}

		void onFree(IComponent *component) override
		{
			if (component == pawn_ || component == this)
			{
				Detach();
			}
		}

		void reset() override
		{
		}

		void free() override
		{
			Detach();
			delete this;
		}

	private:
		void Detach()
		{
			if (!pawn_)
			{
				return;
			}

			core_->getEventDispatcher().removeEventHandler(this);
			pawn_->getEventDispatcher().removeEventHandler(this);

			for (INetwork *network : core_->getNetworks())
			{
				network->getInEventDispatcher().removeEventHandler(this);
			}

			wf::Shutdown();
			pawn_ = nullptr;
		}

		ICore *core_ = nullptr;
		IPawnComponent *pawn_ = nullptr;
	};
}

COMPONENT_ENTRY_POINT()
{
	return new wf_omp::WeaponForgeComponent();
}
