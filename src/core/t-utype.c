/***********************************************************************
**
**  REBOL [R3] Language Interpreter and Run-time Environment
**
**  Copyright 2012 REBOL Technologies
**  REBOL is a trademark of REBOL Technologies
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**  http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
**
************************************************************************
**
**  Module:  t-utype.c
**  Summary: user defined datatype
**  Section: datatypes
**  Author:  Carl Sassenrath
**  Notes:   Implemented by giuliolunati@gmail.com
**
***********************************************************************/

#include "sys-core.h"

#define	SET_UTYPE(v,f) VAL_OBJ_FRAME(v) = (f), VAL_SET(v, REB_UTYPE)


/***********************************************************************
**
*/	REBINT CT_Utype(REBVAL *a, REBVAL *b, REBINT mode)
/*
***********************************************************************/
{
	return FALSE;
}


/***********************************************************************
**
*/	REBFLG MT_Utype(REBVAL *out, REBVAL *data, REBCNT type)
/*
***********************************************************************/
{
	return FALSE;
}


/***********************************************************************
**
*/	REBTYPE(Utype)
/*
***********************************************************************/
{
	REBVAL *value = D_ARG(1);
	REBVAL *arg = D_ARG(2);
	REBINT n;
	REBVAL *val;
	REBSER *obj, *src_obj;
	REBCNT type = 0;

	switch (action) {

	case A_MAKE:
		if (IS_DATATYPE(value) && (REB_UTYPE == VAL_DATATYPE(value))) {

			if (! IS_BLOCK(arg)) Trap_Make(REB_UTYPE, arg);

			// make utype! [init]
			obj = Make_Object(0, VAL_BLK_DATA(arg));
			SET_UTYPE(ds, obj); // GC save
			arg = Do_Bind_Block(obj, arg); // GC-OK
			break; // returns obj
		}

		// make parent ....
		if (IS_UTYPE(value) || IS_OBJECT(value)) {
			src_obj  = VAL_OBJ_FRAME(value);

			// make parent none | []
			if (IS_NONE(arg) || (IS_BLOCK(arg) && IS_EMPTY(arg))) {
				obj = Copy_Block_Values(src_obj, 0, SERIES_TAIL(src_obj), TS_CLONE);
				Rebind_Frame(src_obj, obj);
				break;	// returns obj
			}

			// make parent [...]
			if (IS_BLOCK(arg)) {
				obj = Make_Object(src_obj, VAL_BLK_DATA(arg));
				Rebind_Frame(src_obj, obj);
				SET_UTYPE(ds, obj);
				arg = Do_Bind_Block(obj, arg); // GC-OK
				break; // returns obj
			}

			// make parent object|utype
			if (IS_UTYPE(arg) || IS_OBJECT(arg)) {
				obj = Merge_Frames(src_obj, VAL_OBJ_FRAME(arg));
				break; // returns obj
			}
		}
		Trap_Make(VAL_TYPE(value), value);

	case A_COPY:
		// Note: words are not copied and bindings not changed!
	{
		REBU64 types = 0;
		if (D_REF(ARG_COPY_PART)) Trap0(RE_BAD_REFINES);
		if (D_REF(ARG_COPY_DEEP)) {
			types |= CP_DEEP | (D_REF(ARG_COPY_TYPES) ? 0 : TS_STD_SERIES);
		}
		if D_REF(ARG_COPY_TYPES) {
			arg = D_ARG(ARG_COPY_KINDS);
			if (IS_DATATYPE(arg)) types |= TYPESET(VAL_DATATYPE(arg));
			else types |= VAL_TYPESET(arg);
		}
		VAL_OBJ_FRAME(value) = obj = Copy_Block(VAL_OBJ_FRAME(value), 0);
		if (types != 0) Copy_Deep_Values(obj, 1, SERIES_TAIL(obj), types);
		break; // returns value
	}
	case A_REFLECT:
		action = What_Reflector(arg); // zero on error
		if (action == OF_SPEC) {
			if (!VAL_MOD_SPEC(value)) return R_NONE;
			VAL_OBJ_FRAME(value) = VAL_MOD_SPEC(value);
			VAL_SET(value, REB_OBJECT);
			break;
		}
		// Adjust for compatibility with PICK:
		if (action == OF_VALUES) action = 2;
		else if (action == OF_BODY) action = 3;
		if (action < 1 || action > 3) Trap_Reflect(VAL_TYPE(value), arg);
		if (THROWN(value)) Trap0(RE_THROW_USAGE);
		Set_Block(value, Make_Object_Block(VAL_OBJ_FRAME(value), action));
		break;

	default:
		Trap_Action(VAL_TYPE(value), action);
	}

	VAL_SET(value, REB_UTYPE);
	VAL_OBJ_FRAME(value) = obj;

	DS_RET_VALUE(value);
	return R_RET;
}
