// Health bar (the client's real health is kept at 8000000 + x, so a textdraw
// shows the server-side value) and the damage feed.
#include "core.hpp"
#include "server.hpp"
#include "weapons.hpp"

#include <cmath>
#include <cstdio>

namespace wf
{
	namespace
	{
		constexpr int kFontSprite = 4;
		constexpr float kHealthBase = 8000000.0f;

		bool IsSet(float f)
		{
			return !std::isnan(f);
		}

		float CalculateBar(float width, float max, float value)
		{
			return (width / max) * value;
		}

		void Destroy(int playerid, int &td)
		{
			if (td != kInvalidTextDraw)
			{
				server::PlayerTextDrawDestroy(playerid, td);
				td = kInvalidTextDraw;
			}
		}

		int CreateSprite(int playerid, float x, float y, float w, float h, int color)
		{
			int td = server::CreatePlayerTextDraw(playerid, x, y, "LD_SPAC:white");

			if (td == kInvalidTextDraw)
			{
				Log("WARN: Unable to create player health bar textdraw");
				return kInvalidTextDraw;
			}

			server::PlayerTextDrawTextSize(playerid, td, w, h);
			server::PlayerTextDrawColor(playerid, td, color);
			server::PlayerTextDrawFont(playerid, td, kFontSprite);
			server::PlayerTextDrawShow(playerid, td);
			return td;
		}

		// open.mp formats PlayerTextDrawSetString, so '%' must be escaped there.
		std::string Escape(const std::string &s)
		{
			if (!g_open_mp || s.find('%') == std::string::npos)
			{
				return s;
			}

			std::string out;
			out.reserve(s.size() + 4);

			for (char c : s)
			{
				out += c;
				if (c == '%')
				{
					out += '%';
				}
			}

			return out;
		}
	}

	void GetHealthBarPosition(int playerid, float &x, float &y)
	{
		x = g_config.hb_pos[0];
		y = g_config.hb_pos[1];

		if (ValidPlayer(playerid))
		{
			if (IsSet(P(playerid).hb_pos[0]))
			{
				x = P(playerid).hb_pos[0];
			}
			if (IsSet(P(playerid).hb_pos[1]))
			{
				y = P(playerid).hb_pos[1];
			}
		}
	}

	void GetHealthBarSize(int playerid, float &x, float &y)
	{
		x = g_config.hb_size[0];
		y = g_config.hb_size[1];

		if (ValidPlayer(playerid))
		{
			if (IsSet(P(playerid).hb_size[0]))
			{
				x = P(playerid).hb_size[0];
			}
			if (IsSet(P(playerid).hb_size[1]))
			{
				y = P(playerid).hb_size[1];
			}
		}
	}

	void GetHealthBarPadding(int playerid, float padding[4])
	{
		for (int i = 0; i < 4; ++i)
		{
			padding[i] = g_config.hb_padding[i];

			if (ValidPlayer(playerid) && IsSet(P(playerid).hb_padding[i]))
			{
				padding[i] = P(playerid).hb_padding[i];
			}
		}
	}

	void GetHealthBarColor(int playerid, int &border, int &bg, int &fg)
	{
		border = g_config.hb_border_color;
		bg = g_config.hb_bg_color;
		fg = g_config.hb_fg_color;

		if (ValidPlayer(playerid))
		{
			if (P(playerid).hb_border_color)
			{
				border = P(playerid).hb_border_color;
			}
			if (P(playerid).hb_bg_color)
			{
				bg = P(playerid).hb_bg_color;
			}
			if (P(playerid).hb_fg_color)
			{
				fg = P(playerid).hb_fg_color;
			}
		}
	}

	void UpdateHealthBar(int playerid, bool force)
	{
		if (!ValidPlayer(playerid))
		{
			return;
		}

		Player &p = P(playerid);

		if (!p.connected)
		{
			return;
		}

		int health = static_cast<int>(std::ceil(p.health / p.max_health * 100.0f));
		int armour = static_cast<int>(std::ceil(p.armour / p.max_armour * 100.0f));

		// Make the values reflect what the client should see
		if (p.dying)
		{
			health = 0;
			armour = 0;
		}
		else
		{
			if (health > 100)
			{
				health = 100;
			}
			if (armour > 100)
			{
				armour = 100;
			}
		}

		if (health < 0)
		{
			health = 0;
		}
		if (armour < 0)
		{
			armour = 0;
		}

		if (force)
		{
			p.last_sent_health = -1;
			p.last_sent_armour = -1;
		}
		else if (p.health_bar_visible && p.hb_foreground == kInvalidTextDraw && !p.dying)
		{
			p.last_sent_health = -1;
		}
		else if (health == p.last_sent_health && armour == p.last_sent_armour)
		{
			return;
		}

		SetFakeHealth(playerid, health);
		SetFakeArmour(playerid, armour);

		if (health != p.last_sent_health)
		{
			p.last_sent_health = health;

			server::SetPlayerHealth(playerid, kHealthBase + static_cast<float>(health));

			if (p.health_bar_visible && !p.dying)
			{
				int border, bg, fg;
				float sx, sy, pad[4];

				GetHealthBarSize(playerid, sx, sy);
				GetHealthBarPadding(playerid, pad);
				GetHealthBarColor(playerid, border, bg, fg);

				float width = CalculateBar(sx - pad[1] - pad[3], 100.0f, static_cast<float>(health));
				float height = sy - pad[0] - pad[2];

				if (p.hb_foreground == kInvalidTextDraw)
				{
					float px, py;
					GetHealthBarPosition(playerid, px, py);
					p.hb_foreground = CreateSprite(playerid, px + pad[3], py + pad[0], width, height, fg);
				}
				else
				{
					server::PlayerTextDrawTextSize(playerid, p.hb_foreground, width, height);
					server::PlayerTextDrawShow(playerid, p.hb_foreground);
				}
			}
			else
			{
				Destroy(playerid, p.hb_foreground);
			}
		}

		if (armour != p.last_sent_armour)
		{
			p.last_sent_armour = armour;
			server::SetPlayerArmour(playerid, static_cast<float>(armour));
		}
	}

	void SetHealthBarVisible(int playerid, bool toggle)
	{
		Player &p = P(playerid);

		if (!p.health_bar_enabled)
		{
			toggle = false;
		}

		if (p.health_bar_visible == toggle)
		{
			return;
		}

		p.health_bar_visible = toggle;

		if (toggle)
		{
			int border, bg, fg;
			float px, py, sx, sy, pad[4];

			GetHealthBarPosition(playerid, px, py);
			GetHealthBarSize(playerid, sx, sy);
			GetHealthBarPadding(playerid, pad);
			GetHealthBarColor(playerid, border, bg, fg);

			if (p.hb_border == kInvalidTextDraw)
			{
				p.hb_border = CreateSprite(playerid, px, py, sx, sy, border);
			}
			else
			{
				server::PlayerTextDrawShow(playerid, p.hb_border);
			}

			if (p.hb_background == kInvalidTextDraw)
			{
				p.hb_background = CreateSprite(playerid, px + pad[3], py + pad[0], sx - pad[1] - pad[3], sy - pad[0] - pad[2], bg);
			}
			else
			{
				server::PlayerTextDrawShow(playerid, p.hb_background);
			}

			UpdateHealthBar(playerid, true);
		}
		else
		{
			Destroy(playerid, p.hb_foreground);
			Destroy(playerid, p.hb_border);
			Destroy(playerid, p.hb_background);
		}
	}

	void UpdateHealthBarSize(int playerid)
	{
		Player &p = P(playerid);

		if (!p.connected || !p.health_bar_visible)
		{
			return;
		}

		// Recreate everything so position, size and colors are all applied.
		p.health_bar_visible = false;
		Destroy(playerid, p.hb_foreground);
		Destroy(playerid, p.hb_border);
		Destroy(playerid, p.hb_background);
		SetHealthBarVisible(playerid, true);
	}

	void DestroyHud(int playerid)
	{
		Player &p = P(playerid);

		Destroy(playerid, p.hb_foreground);
		Destroy(playerid, p.hb_border);
		Destroy(playerid, p.hb_background);
		Destroy(playerid, p.feed_given_td);
		Destroy(playerid, p.feed_taken_td);
		p.health_bar_visible = false;
	}

	bool IsDamageFeedActive(int playerid)
	{
		if (ValidPlayer(playerid))
		{
			int mode = P(playerid).feed_mode;
			return mode == 1 || (g_config.damage_feed && mode != 0);
		}

		return g_config.damage_feed;
	}

	namespace
	{
		int CreateFeedTextDraw(int playerid, const float pos[2], int color)
		{
			int td = server::CreatePlayerTextDraw(playerid, pos[0], pos[1], "_");

			if (td == kInvalidTextDraw)
			{
				Log("WARN: Unable to create damage feed textdraw");
				return td;
			}

			server::PlayerTextDrawLetterSize(playerid, td, 0.2f, 0.9f);
			server::PlayerTextDrawColor(playerid, td, color);
			server::PlayerTextDrawAlignment(playerid, td, 2);
			server::PlayerTextDrawSetOutline(playerid, td, 1);
			server::PlayerTextDrawBackgroundColor(playerid, td, 0x0000001A);
			return td;
		}

		std::string WeaponLabel(int weapon)
		{
			return weapon == -1 ? std::string("Multiple") : weapons::Name(weapon);
		}

		void DamageFeedUpdateText(int playerid)
		{
			Player &p = P(playerid);
			char line[192];
			std::string buf;

			for (const FeedHit &hit : p.feed_given)
			{
				if (!hit.tick)
				{
					break;
				}

				std::string weapon = WeaponLabel(hit.weapon);

				if (hit.issuer == kInvalidPlayer)
				{
					std::snprintf(line, sizeof(line), "%s +%.2f~n~", weapon.c_str(), hit.amount + 0.009f);
				}
				else
				{
					float hp = ValidPlayer(hit.issuer) ? P(hit.issuer).health : 0.0f;
					std::snprintf(line, sizeof(line), "%s - %s +%.2f (%.2f)~n~", hit.name.c_str(), weapon.c_str(), hit.amount + 0.009f, hp);
				}

				buf += line;
			}

			if (p.feed_given_td != kInvalidTextDraw)
			{
				if (!buf.empty())
				{
					server::PlayerTextDrawSetString(playerid, p.feed_given_td, Escape(buf).c_str());
					server::PlayerTextDrawShow(playerid, p.feed_given_td);
				}
				else
				{
					server::PlayerTextDrawHide(playerid, p.feed_given_td);
				}
			}

			buf.clear();
			int damagedid = p.spectating == kInvalidPlayer ? playerid : p.spectating;
			float own_health = ValidPlayer(damagedid) ? P(damagedid).health : 0.0f;

			for (const FeedHit &hit : p.feed_taken)
			{
				if (!hit.tick)
				{
					break;
				}

				std::string weapon = WeaponLabel(hit.weapon);

				if (hit.issuer == kInvalidPlayer)
				{
					std::snprintf(line, sizeof(line), "%s -%.2f (%.2f)~n~", weapon.c_str(), hit.amount + 0.009f, own_health);
				}
				else
				{
					std::snprintf(line, sizeof(line), "%s - %s -%.2f (%.2f)~n~", hit.name.c_str(), weapon.c_str(), hit.amount + 0.009f, own_health);
				}

				buf += line;
			}

			if (p.feed_taken_td != kInvalidTextDraw)
			{
				if (!buf.empty())
				{
					server::PlayerTextDrawSetString(playerid, p.feed_taken_td, Escape(buf).c_str());
					server::PlayerTextDrawShow(playerid, p.feed_taken_td);
				}
				else
				{
					server::PlayerTextDrawHide(playerid, p.feed_taken_td);
				}
			}
		}

		// Removes expired hits; returns true if anything changed.
		bool Expire(FeedHit (&arr)[kFeedHeight], uint32_t tick, uint32_t &lowest_tick)
		{
			for (int i = 0; i < kFeedHeight; ++i)
			{
				if (!arr[i].tick)
				{
					break;
				}

				if (TickDiff(tick, arr[i].tick) >= g_config.feed_hide_delay)
				{
					for (int j = i; j < kFeedHeight; ++j)
					{
						arr[j].tick = 0;
					}
					return true;
				}

				if (TickDiff(arr[i].tick, lowest_tick) < 0)
				{
					lowest_tick = arr[i].tick;
				}
			}

			return false;
		}

		void DamageFeedAddHit(FeedHit (&arr)[kFeedHeight], int playerid, int issuerid, float amount, int weapon)
		{
			if (!IsDamageFeedActive(playerid))
			{
				return;
			}

			uint32_t tick = Tick();
			int idx = -1;

			for (int i = 0; i < kFeedHeight; ++i)
			{
				if (!arr[i].tick)
				{
					break;
				}

				if (TickDiff(tick, arr[i].tick) >= g_config.feed_hide_delay)
				{
					for (int j = i; j < kFeedHeight; ++j)
					{
						arr[j].tick = 0;
					}
					break;
				}

				// Merge it and move it on top below
				if (arr[i].issuer == issuerid)
				{
					amount += arr[i].amount;
					idx = i;
					break;
				}
			}

			if (idx == -1)
			{
				idx = 0;

				// Insert it at the top
				for (int i = kFeedHeight - 1; i >= 1; --i)
				{
					arr[i] = arr[i - 1];
				}
			}

			arr[idx].tick = tick;
			arr[idx].amount = amount;
			arr[idx].issuer = issuerid;
			arr[idx].weapon = weapon;

			if (ValidPlayer(issuerid))
			{
				arr[idx].name = server::GetPlayerName(issuerid);
			}
			else
			{
				arr[idx].name.clear();
			}

			DamageFeedUpdate(playerid, true);
		}
	}

	void DamageFeedUpdate(int playerid, bool modified)
	{
		Player &p = P(playerid);

		if (!p.connected)
		{
			return;
		}

		if (!IsDamageFeedActive(playerid))
		{
			Destroy(playerid, p.feed_given_td);
			Destroy(playerid, p.feed_taken_td);
			timers::Stop(timers::FEED_UPDATE, playerid);
			return;
		}

		if (p.feed_given_td == kInvalidTextDraw)
		{
			p.feed_given_td = CreateFeedTextDraw(playerid, g_config.feed_given_pos, g_config.feed_given_color);
		}

		if (p.feed_taken_td == kInvalidTextDraw)
		{
			p.feed_taken_td = CreateFeedTextDraw(playerid, g_config.feed_taken_pos, g_config.feed_taken_color);
		}

		uint32_t tick = Tick();
		uint32_t lowest_tick = tick + 1;

		modified = Expire(p.feed_given, tick, lowest_tick) || modified;
		modified = Expire(p.feed_taken, tick, lowest_tick) || modified;

		timers::Stop(timers::FEED_UPDATE, playerid);

		int since = TickDiff(tick, p.feed_update_tick);

		if (since < g_config.feed_max_update_rate && modified)
		{
			timers::Start(timers::FEED_UPDATE, playerid, g_config.feed_max_update_rate - since);
			return;
		}

		if (lowest_tick == tick + 1)
		{
			modified = true;
		}
		else
		{
			timers::Start(timers::FEED_UPDATE, playerid, g_config.feed_hide_delay - TickDiff(tick, lowest_tick) + 10);
		}

		if (modified)
		{
			DamageFeedUpdateText(playerid);
			p.feed_update_tick = tick;
		}
	}

	void DamageFeedAddHitGiven(int playerid, int damagedid, float amount, int weapon)
	{
		for (int i = 0; i < g_max_players; ++i)
		{
			if (i != playerid && P(i).connected && P(i).spectating == playerid)
			{
				DamageFeedAddHit(P(i).feed_given, i, damagedid, amount, weapon);
			}
		}

		DamageFeedAddHit(P(playerid).feed_given, playerid, damagedid, amount, weapon);
	}

	void DamageFeedAddHitTaken(int playerid, int issuerid, float amount, int weapon)
	{
		for (int i = 0; i < g_max_players; ++i)
		{
			if (i != playerid && P(i).connected && P(i).spectating == playerid)
			{
				DamageFeedAddHit(P(i).feed_taken, i, issuerid, amount, weapon);
			}
		}

		DamageFeedAddHit(P(playerid).feed_taken, playerid, issuerid, amount, weapon);
	}
}
