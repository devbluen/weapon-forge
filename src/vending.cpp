// Custom vending machines: the default ones heal client-side (which does not
// work with server-side health), so they are replaced by objects and their
// use is handled here, like weapon-config does.
#include "core.hpp"
#include "scripts.hpp"
#include "server.hpp"

#include <cmath>

namespace wf
{
	namespace
	{
		struct VendingMachine
		{
			int model;
			int interior;
			float x, y, z;
			float rx, ry, rz;
			float front_x, front_y;
		};

		const VendingMachine kMachines[] = {
			{955, 0, -862.82f, 1536.60f, 21.98f, 0.00f, 0.00f, 180.00f, -862.84f, 1537.60f},
			{956, 0, 2271.72f, -76.46f, 25.96f, 0.00f, 0.00f, 0.00f, 2271.72f, -77.46f},
			{955, 0, 1277.83f, 372.51f, 18.95f, 0.00f, 0.00f, 64.00f, 1278.73f, 372.07f},
			{956, 0, 662.42f, -552.16f, 15.71f, 0.00f, 0.00f, 180.00f, 662.41f, -551.16f},
			{955, 0, 201.01f, -107.61f, 0.89f, 0.00f, 0.00f, 270.00f, 200.01f, -107.63f},
			{955, 0, -253.74f, 2597.95f, 62.24f, 0.00f, 0.00f, 90.00f, -252.74f, 2597.95f},
			{956, 0, -253.74f, 2599.75f, 62.24f, 0.00f, 0.00f, 90.00f, -252.74f, 2599.75f},
			{956, 0, -76.03f, 1227.99f, 19.12f, 0.00f, 0.00f, 90.00f, -75.03f, 1227.99f},
			{955, 0, -14.70f, 1175.35f, 18.95f, 0.00f, 0.00f, 180.00f, -14.72f, 1176.35f},
			{1977, 7, 316.87f, -140.35f, 998.58f, 0.00f, 0.00f, 270.00f, 315.87f, -140.36f},
			{1775, 17, 373.82f, -178.14f, 1000.73f, 0.00f, 0.00f, 0.00f, 373.82f, -179.14f},
			{1776, 17, 379.03f, -178.88f, 1000.73f, 0.00f, 0.00f, 270.00f, 378.03f, -178.90f},
			{1775, 17, 495.96f, -24.32f, 1000.73f, 0.00f, 0.00f, 180.00f, 495.95f, -23.32f},
			{1776, 17, 500.56f, -1.36f, 1000.73f, 0.00f, 0.00f, 0.00f, 500.56f, -2.36f},
			{1775, 17, 501.82f, -1.42f, 1000.73f, 0.00f, 0.00f, 0.00f, 501.82f, -2.42f},
			{956, 0, -1455.11f, 2591.66f, 55.23f, 0.00f, 0.00f, 180.00f, -1455.13f, 2592.66f},
			{955, 0, 2352.17f, -1357.15f, 23.77f, 0.00f, 0.00f, 90.00f, 2353.17f, -1357.15f},
			{955, 0, 2325.97f, -1645.13f, 14.21f, 0.00f, 0.00f, 0.00f, 2325.97f, -1646.13f},
			{956, 0, 2139.51f, -1161.48f, 23.35f, 0.00f, 0.00f, 87.00f, 2140.51f, -1161.53f},
			{956, 0, 2153.23f, -1016.14f, 62.23f, 0.00f, 0.00f, 127.00f, 2154.03f, -1015.54f},
			{955, 0, 1928.73f, -1772.44f, 12.94f, 0.00f, 0.00f, 90.00f, 1929.73f, -1772.44f},
			{1776, 1, 2222.36f, 1602.64f, 1000.06f, 0.00f, 0.00f, 90.00f, 2223.36f, 1602.64f},
			{1775, 1, 2222.20f, 1606.77f, 1000.05f, 0.00f, 0.00f, 90.00f, 2223.20f, 1606.77f},
			{1775, 1, 2155.90f, 1606.77f, 1000.05f, 0.00f, 0.00f, 90.00f, 2156.90f, 1606.77f},
			{1775, 1, 2209.90f, 1607.19f, 1000.05f, 0.00f, 0.00f, 270.00f, 2208.90f, 1607.17f},
			{1776, 1, 2155.84f, 1607.87f, 1000.06f, 0.00f, 0.00f, 90.00f, 2156.84f, 1607.87f},
			{1776, 1, 2202.45f, 1617.00f, 1000.06f, 0.00f, 0.00f, 180.00f, 2202.43f, 1618.00f},
			{1776, 1, 2209.24f, 1621.21f, 1000.06f, 0.00f, 0.00f, 0.00f, 2209.24f, 1620.21f},
			{1776, 3, 330.67f, 178.50f, 1020.07f, 0.00f, 0.00f, 0.00f, 330.67f, 177.50f},
			{1776, 3, 331.92f, 178.50f, 1020.07f, 0.00f, 0.00f, 0.00f, 331.92f, 177.50f},
			{1776, 3, 350.90f, 206.08f, 1008.47f, 0.00f, 0.00f, 90.00f, 351.90f, 206.08f},
			{1776, 3, 361.56f, 158.61f, 1008.47f, 0.00f, 0.00f, 180.00f, 361.54f, 159.61f},
			{1776, 3, 371.59f, 178.45f, 1020.07f, 0.00f, 0.00f, 0.00f, 371.59f, 177.45f},
			{1776, 3, 374.89f, 188.97f, 1008.47f, 0.00f, 0.00f, 0.00f, 374.89f, 187.97f},
			{1775, 2, 2576.70f, -1284.43f, 1061.09f, 0.00f, 0.00f, 270.00f, 2575.70f, -1284.44f},
			{1775, 15, 2225.20f, -1153.42f, 1025.90f, 0.00f, 0.00f, 270.00f, 2224.20f, -1153.43f},
			{955, 0, 1154.72f, -1460.89f, 15.15f, 0.00f, 0.00f, 270.00f, 1153.72f, -1460.90f},
			{956, 0, 2480.85f, -1959.27f, 12.96f, 0.00f, 0.00f, 180.00f, 2480.84f, -1958.27f},
			{955, 0, 2060.11f, -1897.64f, 12.92f, 0.00f, 0.00f, 0.00f, 2060.11f, -1898.64f},
			{955, 0, 1729.78f, -1943.04f, 12.94f, 0.00f, 0.00f, 0.00f, 1729.78f, -1944.04f},
			{956, 0, 1634.10f, -2237.53f, 12.89f, 0.00f, 0.00f, 0.00f, 1634.10f, -2238.53f},
			{955, 0, 1789.21f, -1369.26f, 15.16f, 0.00f, 0.00f, 270.00f, 1788.21f, -1369.28f},
			{956, 0, -2229.18f, 286.41f, 34.70f, 0.00f, 0.00f, 180.00f, -2229.20f, 287.41f},
			{955, 256, -1980.78f, 142.66f, 27.07f, 0.00f, 0.00f, 270.00f, -1981.78f, 142.64f},
			{955, 256, -2118.96f, -423.64f, 34.72f, 0.00f, 0.00f, 255.00f, -2119.93f, -423.40f},
			{955, 256, -2118.61f, -422.41f, 34.72f, 0.00f, 0.00f, 255.00f, -2119.58f, -422.17f},
			{955, 256, -2097.27f, -398.33f, 34.72f, 0.00f, 0.00f, 180.00f, -2097.29f, -397.33f},
			{955, 256, -2092.08f, -490.05f, 34.72f, 0.00f, 0.00f, 0.00f, -2092.08f, -491.05f},
			{955, 256, -2063.27f, -490.05f, 34.72f, 0.00f, 0.00f, 0.00f, -2063.27f, -491.05f},
			{955, 256, -2005.64f, -490.05f, 34.72f, 0.00f, 0.00f, 0.00f, -2005.64f, -491.05f},
			{955, 256, -2034.46f, -490.05f, 34.72f, 0.00f, 0.00f, 0.00f, -2034.46f, -491.05f},
			{955, 256, -2068.56f, -398.33f, 34.72f, 0.00f, 0.00f, 180.00f, -2068.58f, -397.33f},
			{955, 256, -2039.85f, -398.33f, 34.72f, 0.00f, 0.00f, 180.00f, -2039.86f, -397.33f},
			{955, 256, -2011.14f, -398.33f, 34.72f, 0.00f, 0.00f, 180.00f, -2011.15f, -397.33f},
			{955, 2048, -1350.11f, 492.28f, 10.58f, 0.00f, 0.00f, 90.00f, -1349.11f, 492.28f},
			{956, 2048, -1350.11f, 493.85f, 10.58f, 0.00f, 0.00f, 90.00f, -1349.11f, 493.85f},
			{955, 0, 2319.99f, 2532.85f, 10.21f, 0.00f, 0.00f, 0.00f, 2319.99f, 2531.85f},
			{956, 0, 2845.72f, 1295.04f, 10.78f, 0.00f, 0.00f, 0.00f, 2845.72f, 1294.04f},
			{955, 0, 2503.14f, 1243.69f, 10.21f, 0.00f, 0.00f, 180.00f, 2503.12f, 1244.69f},
			{956, 0, 2647.69f, 1129.66f, 10.21f, 0.00f, 0.00f, 0.00f, 2647.69f, 1128.66f},
			{1209, 0, -2420.21f, 984.57f, 44.29f, 0.00f, 0.00f, 90.00f, -2419.21f, 984.57f},
			{1302, 0, -2420.17f, 985.94f, 44.29f, 0.00f, 0.00f, 90.00f, -2419.17f, 985.94f},
			{955, 0, 2085.77f, 2071.35f, 10.45f, 0.00f, 0.00f, 90.00f, 2086.77f, 2071.35f},
			{956, 0, 1398.84f, 2222.60f, 10.42f, 0.00f, 0.00f, 180.00f, 1398.82f, 2223.60f},
			{956, 0, 1659.46f, 1722.85f, 10.21f, 0.00f, 0.00f, 0.00f, 1659.46f, 1721.85f},
			{955, 0, 1520.14f, 1055.26f, 10.00f, 0.00f, 0.00f, 270.00f, 1519.14f, 1055.24f},
			{1775, 6, -19.03f, -57.83f, 1003.63f, 0.00f, 0.00f, 180.00f, -19.05f, -56.83f},
			{1775, 18, -16.11f, -91.64f, 1003.63f, 0.00f, 0.00f, 180.00f, -16.13f, -90.64f},
			{1775, 16, -15.10f, -140.22f, 1003.63f, 0.00f, 0.00f, 180.00f, -15.11f, -139.22f},
			{1775, 17, -32.44f, -186.69f, 1003.63f, 0.00f, 0.00f, 180.00f, -32.46f, -185.69f},
			{1775, 16, -35.72f, -140.22f, 1003.63f, 0.00f, 0.00f, 180.00f, -35.74f, -139.22f},
			{1776, 6, -36.14f, -57.87f, 1003.63f, 0.00f, 0.00f, 180.00f, -36.16f, -56.87f},
			{1776, 18, -17.54f, -91.71f, 1003.63f, 0.00f, 0.00f, 180.00f, -17.56f, -90.71f},
			{1776, 16, -16.53f, -140.29f, 1003.63f, 0.00f, 0.00f, 180.00f, -16.54f, -139.29f},
			{1776, 17, -33.87f, -186.76f, 1003.63f, 0.00f, 0.00f, 180.00f, -33.89f, -185.76f},
		};

		constexpr int kMachineCount = sizeof(kMachines) / sizeof(kMachines[0]);
		int g_objects[kMachineCount];
		bool g_created = false;
	}

	void CreateVendingMachines()
	{
		DestroyVendingMachines();

		for (int i = 0; i < kMachineCount; ++i)
		{
			const VendingMachine &m = kMachines[i];
			g_objects[i] = server::CreateObject(m.model, m.x, m.y, m.z, m.rx, m.ry, m.rz, 0.0f);
		}

		g_created = true;
	}

	void DestroyVendingMachines()
	{
		if (!g_created)
		{
			return;
		}

		for (int i = 0; i < kMachineCount; ++i)
		{
			if (g_objects[i] != kInvalidObject)
			{
				server::DestroyObject(g_objects[i]);
				g_objects[i] = kInvalidObject;
			}
		}

		g_created = false;
	}

	void RemoveDefaultVendingMachines(int playerid)
	{
		static const int kModels[] = {955, 956, 1209, 1302, 1775, 1776, 1977};

		for (int model : kModels)
		{
			server::RemoveBuildingForPlayer(playerid, model, 0.0f, 0.0f, 0.0f, 20000.0f);
		}

		// Make sure they're all gone
		for (const VendingMachine &m : kMachines)
		{
			server::RemoveBuildingForPlayer(playerid, m.model, m.x, m.y, m.z, 1.0f);
		}
	}

	bool TryUseVendingMachine(int playerid)
	{
		Player &p = P(playerid);
		bool failed = server::GetPlayerMoney(playerid) <= 0 || p.health >= p.max_health;

		float x, y, z;
		server::GetPlayerPos(playerid, x, y, z);

		for (const VendingMachine &m : kMachines)
		{
			if (std::fabs(z - m.z) > 1.5f)
			{
				continue;
			}

			if (!server::IsPlayerInRangeOfPoint(playerid, 0.5f, m.front_x, m.front_y, z))
			{
				continue;
			}

			if (failed)
			{
				server::PlayerPlaySound(playerid, 1055, 0.0f, 0.0f, 0.0f);
				return false;
			}

			float health_given = 35.0f;

			if (scripts::Call("OnPlayerUseVendingMachine").Int(playerid).RefFloat(health_given).Exec(1, true))
			{
				timers::Args args;
				args.f = health_given;
				timers::Start(timers::VENDING_USE, playerid, 2500, args);

				server::SetPlayerFacingAngle(playerid, m.rz);
				server::SetPlayerPos(playerid, m.front_x, m.front_y, z);
				server::ApplyAnimation(playerid, "VENDING", "VEND_USE", 4.1f, false, false, true, false, 0, 1);
				server::PlayerPlaySound(playerid, 42600, 0.0f, 0.0f, 0.0f);
				return true;
			}

			server::PlayerPlaySound(playerid, 1055, 0.0f, 0.0f, 0.0f);
			return false;
		}

		return false;
	}

	void FinishVendingMachine(int playerid, float health_given)
	{
		Player &p = P(playerid);

		if (server::GetPlayerState(playerid) == PLAYER_STATE_ONFOOT && !p.dying)
		{
			float health = p.health + health_given;

			if (health > p.max_health)
			{
				health = p.max_health;
			}

			SetHealth(playerid, health);
		}
	}
}
