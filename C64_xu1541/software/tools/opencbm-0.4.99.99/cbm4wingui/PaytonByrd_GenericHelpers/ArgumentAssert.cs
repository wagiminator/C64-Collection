using System;
using System.Collections.Generic;
using System.Text;

namespace PaytonByrd.GenericHelpers
{
    public static class ArgumentAssert
    {
        private static void IsNotNull(object pv_objValue, string pv_strName)
        {
            if (pv_objValue == null)
            {
                throw new ArgumentNullException(
                    pv_strName,
                    "Argument may not be null.");
            }
        }

        public static void IsGenericNotNull<T>(T pv_tValue, string pv_strName)
        {
            Type type = typeof(T);

            if (type.IsClass ||
                GenericsHelper.IsNullable(type))
            {
                IsNotNull(pv_tValue as object, pv_strName);
            }
        }

        public static void IsStringNotNullNorEmpty(string pv_strValue, string pv_strName)
        {
            IsGenericNotNull<string>(pv_strValue, pv_strName);

            if (pv_strValue == string.Empty)
            {
                throw new ArgumentException(
                    "Argument may not be empty.",
                    pv_strName);
            }
        }

        public static void IsGenericInRange<T>(T pv_tValue, string pv_strName, T pv_tLow, T pv_tHigh)
        {
            IsGenericNotNull<T>(pv_tValue, pv_strName);

            IsInRange(pv_tValue, pv_strName, pv_tLow, pv_tHigh);
        }

        private static void IsInRange(
            object pv_objValue,
            string pv_strName,
            object pv_objLow,
            object pv_objHigh)
        {            
            if (pv_objValue is IComparable)
            {
                IComparable objValue = (IComparable)pv_objValue;
                IComparable objLow = (IComparable)pv_objLow;
                IComparable objHigh = (IComparable)pv_objHigh;

                if (!(objValue.CompareTo(objLow) >= 0
                    && objValue.CompareTo(objHigh) <= 0))
                {
                    throw new ArgumentOutOfRangeException(
                        pv_strName, pv_objValue,
                        string.Format("Argument is not between {0} and {1}", pv_objLow, pv_objHigh));
                }
            }
            else
            {
                throw new ArgumentException(
                    "Argument does not implement IComparable.",
                    pv_strName);
            }
        }
    }
}
