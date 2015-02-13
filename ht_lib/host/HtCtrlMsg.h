/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdint.h>

#pragma once

namespace Ht {

	struct CHtCtrlMsg {
		CHtCtrlMsg() {
			m_data = 0;
		}

		CHtCtrlMsg(uint8_t msgType, uint64_t msgData) {
			m_valid = 1; m_msgType = msgType; m_msgData = msgData;
		}

		bool IsValid() {
			return m_valid == 1;
		}
		uint8_t GetMsgType() {
			return m_msgType;
		}
		uint64_t GetMsgData() {
			return m_msgData;
		}
		void Clear() {
			m_data = 0;
		}

	public:
		union {
			struct {
				uint64_t volatile m_valid : 1;
				uint64_t volatile m_msgType : 7;
				uint64_t volatile m_msgData : 56;
			};
			uint64_t volatile m_data;
		};
	};

} // namespace Ht
