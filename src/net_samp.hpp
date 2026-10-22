#pragma once

namespace net_samp
{
	void Init(void **ppData);
	// Retried every tick until the server's RakServer exists
	void TryInstall();
	void Remove();
}
