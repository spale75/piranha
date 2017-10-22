#!/bin/sh -e

# /*******************************************************************************/
# /*                                                                             */
# /*  Copyright 2004-2017 Pascal Gloor                                           */
# /*                                                                             */
# /*  Licensed under the Apache License, Version 2.0 (the "License");            */
# /*  you may not use this file except in compliance with the License.           */
# /*  You may obtain a copy of the License at                                    */
# /*                                                                             */
# /*     http://www.apache.org/licenses/LICENSE-2.0                              */
# /*                                                                             */
# /*  Unless required by applicable law or agreed to in writing, software        */
# /*  distributed under the License is distributed on an "AS IS" BASIS,          */
# /*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
# /*  See the License for the specific language governing permissions and        */
# /*  limitations under the License.                                             */
# /*                                                                             */
# /*******************************************************************************/



# Piranha source cleaning script

for item in bin obj *.core utils/piranhactl
do
	if [ -d "$item" ]
	then
		echo "deleting directory $item..."
		rm -rf $item
	elif [ -f "$item" ]
	then
		echo "deleting file $item..."
		rm -f $item
	fi
done

echo "cleaned";
