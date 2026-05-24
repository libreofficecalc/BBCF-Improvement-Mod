#pragma once

#include "UpdateState.h"

#include <string>

namespace Updater
{
	class UpdateStateStore
	{
	public:
		UpdateStateStore();
		explicit UpdateStateStore(const std::wstring& statePath);

		bool Load(UpdateState& state) const;
		bool Save(const UpdateState& state) const;

		const std::wstring& GetStatePath() const;

	private:
		std::wstring m_statePath;

		bool EnsureStateDirectory() const;
	};
}
