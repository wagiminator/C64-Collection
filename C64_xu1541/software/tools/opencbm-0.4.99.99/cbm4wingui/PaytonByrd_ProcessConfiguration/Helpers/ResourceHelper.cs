using System;
using System.Collections.Generic;
using System.Text;

namespace PaytonByrd.ProcessConfiguration.Helpers
{
    public class ResourceHelper
    {
        private const string DEFAULT = "en-us";
        private readonly static string m_strUserCulture = null;
        private readonly static System.Resources.ResourceManager m_objResourceManager = null;

        static ResourceHelper()
        {
            m_strUserCulture = DEFAULT;

            System.Globalization.CultureInfo objCultureInfo =
                new System.Globalization.CultureInfo(m_strUserCulture);

            System.Threading.Thread.CurrentThread.CurrentCulture = objCultureInfo;
            System.Threading.Thread.CurrentThread.CurrentUICulture = objCultureInfo;

            m_objResourceManager = new System.Resources.ResourceManager(
                string.Format("PaytonByrd.ProcessConfiguration.{0}", m_strUserCulture),
                System.Reflection.Assembly.GetExecutingAssembly());
        }

        public static string GetResource(string pv_strResourceName)
        {
            PaytonByrd.GenericHelpers.ArgumentAssert.IsStringNotNullNorEmpty(pv_strResourceName, "pv_strResourceName");

            return m_objResourceManager.GetString(pv_strResourceName);
        }
    }
}
