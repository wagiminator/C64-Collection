using System;
using System.Collections.Generic;
using System.Text;

namespace PaytonByrd.GenericHelpers
{
    public static class GenericsHelper
    {
        public static bool IsGeneric(object pv_objValue)
        {
            return IsGeneric(pv_objValue.GetType());
        }

        public static bool IsGeneric(Type pv_objType)
        {
            return pv_objType.IsGenericType;
        }

        public static bool IsNullable(Type pv_objType)
        {
            bool blnResult;
            Type objGenericType = null;

            if (!IsGeneric(pv_objType))
            {
                blnResult = false;
            }
            else
            {
                objGenericType = pv_objType.GetGenericTypeDefinition();

                blnResult = objGenericType.Equals(typeof(Nullable<>));
            }

            return blnResult;
        }
    }
}
