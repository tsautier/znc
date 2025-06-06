#
# Copyright (C) 2004-2025 ZNC, see the NOTICE file for details.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

file(READ "${source}" content)
string(REPLACE "#include \"defines.h\"" "#include <znc/defines.h>" content "${content}")
string(REPLACE "#include \"Csocket.h\"" "#include <znc/Csocket.h>" content "${content}")
file(WRITE "${destination}" "${content}")
