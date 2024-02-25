//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

enum RTDBResponse {
	RC_OK = 1;
	RC_VARIABLE_EXISTS = 2;
	RC_VARIABLE_DESCRIPTION_TOO_LONG = 3;
	RC_VALUE_TOO_LONG = 4;
	RC_WRONG_REPETITION_NUMBER = 5;
	RC_VARIABLE_DOES_NOT_EXIST = 6;
	RC_VARIABLE_IS_NOT_PRODUCER = 7;
	RC_VARIABLE_BEING_DELETED = 8;
};

enum RTDBOperations {
    RTDB_CREATE = 1;
    RTDB_DELETE = 2;
    RTDB_UPDATE = 3;
    RTDB_READ = 4;
};

//Value returned to the application module from the RTDB
message RTDBResponseCode {
    int responseCode @enum(RTDBResponse);
    int responseKind @enum(RTDBOperations);
};
