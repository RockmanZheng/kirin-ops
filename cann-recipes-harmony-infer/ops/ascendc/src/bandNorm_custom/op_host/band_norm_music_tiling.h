/**
 * ************************************************************************************
 * Copyright (c) 2025 Huibin Lin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ************************************************************************************
 *
 * @File    :   band_norm_music_tiling.h
 * @Time    :   2025/05/11 17:19:55
 * @Author  :   Huibin Lin
 * @Contact :   steelbin@126.com
 * @Version :   1.0
 * @Desc    :   Set the tiling data structure for the BandNorm Music operator.
 * ************************************************************************************
 */

#ifndef OP_HOST_BANDNORMMUSIC_TILING_H
#define OP_HOST_BANDNORMMUSIC_TILING_H
#include "register/tilingdata_base.h"

namespace optiling {

BEGIN_TILING_DATA_DEF(BandNormMusicTilingData)
    TILING_DATA_FIELD_DEF(uint32_t, bLen);
    TILING_DATA_FIELD_DEF(uint32_t, tLen);
    TILING_DATA_FIELD_DEF(uint32_t, bandWide);
    TILING_DATA_FIELD_DEF(uint32_t, blockElement);
    TILING_DATA_FIELD_DEF(uint32_t, blockHalfAlignSize);
    TILING_DATA_FIELD_DEF(uint32_t, blockHalfAlignElement);
    TILING_DATA_FIELD_DEF(uint32_t, blockFloatAlignSize);
    TILING_DATA_FIELD_DEF(uint32_t, blockFloatAlignElement);

END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(BandNormMusic, BandNormMusicTilingData)
}

#endif // OP_HOST_BANDNORMMUSIC_TILING_H
