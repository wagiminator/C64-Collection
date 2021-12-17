using System;
using System.Collections.Generic;
using System.Text;

namespace PaytonByrd.GenericHelpers
{
    public static class ArgumentAssertCopy
    {
        public static void IsNotNull(object pv_tValue, string pv_strName)
        {
            if (pv_tValue == null)
            {
                throw new ArgumentNullException(
                    pv_strName,
                    "Argument may not be null.");
            }
        }

        public static void IsNotNull<T>(Nullable<T> pv_tValue, string pv_strName) where T : struct
        {
            if (!pv_tValue.HasValue)
            {
                throw new ArgumentNullException(
                    pv_strName,
                    "Argument may not be null.");
            }
        }

        public static void IsNotNull<T>(T pv_tValue, string pv_strName) where T : class
        {
            if (pv_tValue == null)
            {
                throw new ArgumentNullException(
                    pv_strName,
                    "Argument may not be null.");
            }
        }

        public static void IsNotNullNorEmpty(string pv_strValue, string pv_strName)
        {
            IsNotNull<string>(pv_strValue, pv_strName);

            if (pv_strValue == string.Empty)
            {
                throw new ArgumentException(
                    "Argument may not be empty.",
                    pv_strName);
            }
        }

        public static void IsInRange<T>(T pv_tValue, string pv_strName, T pv_tLow, T pv_tHigh) where T : class
        {
            Type objValueType = typeof(T);

            if (objValueType.IsClass ||
                GenericsHelper.IsNullable(objValueType)
            )
            {
                IsReferenceTypeInRange<T>(
                    pv_tValue, pv_strName, pv_tHigh, pv_tLow);
            }
            else
            {
                IsValueTypeInRange<T>(
                    pv_tValue, pv_strName, pv_tHigh, pv_tLow);
            }

        }

        private static void IsValueTypeInRange<T>(
            T pv_tValue, 
            string pv_strName, 
            T pv_tLow, 
            T pv_tHigh)
                where T : struct
        {
            if (pv_tValue is IComparable)
            {
                CompareRange(pv_tValue, pv_strName, pv_tHigh, pv_tLow);
            }
            else
            {
                throw new ArgumentException(
                    "Argument does not implement IComparable.",
                    pv_strName);
            }
        }

        private static void IsReferenceTypeInRange<T>(
            Nullable<T> pv_tValue, 
            string pv_strName, 
            Nullable<T> pv_tLow, 
            Nullable<T> pv_tHigh) 
                where T : struct
        {
            IsNullableInRange(
                pv_tValue, pv_strName, pv_tHigh, pv_tLow);
        }

        private static void IsReferenceTypeInRange<T>(
            T pv_tValue, 
            string pv_strName, 
            T pv_tLow, 
            T pv_tHigh) 
                where T : class
        {
            IsNullableInRange(
                pv_tValue, pv_strName, pv_tHigh, pv_tLow);
        }

        private static void IsNullableInRange(
            object pv_tValue, 
            string pv_strName, 
            object pv_tLow, 
            object pv_tHigh)
        {
            IsNotNull(pv_tValue, pv_strName);

            if (pv_tValue is IComparable)
            {
                CompareRange(pv_tValue, pv_strName, pv_tHigh, pv_tLow);
            }
            else
            {
                throw new ArgumentException(
                    "Argument does not implement IComparable.",
                    pv_strName);
            }
        }

        private static void CompareRange(
            object pv_tValue,
            string pv_strName,
            object pv_tLow,
            object pv_tHigh)
        {
            IComparable objValue = (IComparable)pv_tValue;
            IComparable objLow = (IComparable)pv_tLow;
            IComparable objHigh = (IComparable)pv_tHigh;

            if (!(objValue.CompareTo(objLow) >= 0
                && objValue.CompareTo(objHigh) <= 0))
            {
                throw new ArgumentOutOfRangeException(
                    pv_strName, pv_tValue,
                    string.Format("Argument is not between {0} and {1}", pv_tLow, pv_tHigh));
            }
        }
    }
}
