using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

using PaytonByrd.GenericHelpers;

namespace PaytonByrd.ProcessConfiguration
{
    public class ProcessParameterAttribute : System.Attribute
    {
        private const string CANNOT_BE_NULL_RESOURCE_ID = "ERROR_MSG_CANNOT_BE_NULL";
        private const string DOES_NOT_MATCH_EXPRESSION_RESOURCE_ID = "ERROR_MSG_DOES_NOT_MATCH_EXPRESSION";

        private string m_strParameterName = null;

        public string ParameterName
        {
            get { return m_strParameterName; }
        }

        private string m_strParameterAbbreviation = null;

        public string ParameterAbbreviation
        {
            get { return m_strParameterAbbreviation; }
        }

        private bool m_blnParameterRequired = false;

        public bool ParameterRequired
        {
            get { return m_blnParameterRequired; }
        }

        private string m_strValidationExpression = ".*";

        public string ValidationExpression
        {
            get { return m_strValidationExpression; }
        }
        
        private string m_strParameterHelpResource = null;

        public string ParameterHelpResource
        {
            get { return m_strParameterHelpResource; }
        }

        /// <summary>
        /// Attribute to describe how to process a property as a command line parameter.
        /// </summary>
        /// <param name="pv_strParameterName">Long name of the parameter. (Ex: --help)</param>
        /// <param name="pv_strParameterAbbreviation">Short name of the parameter. (Ex: -h)</param>
        /// <param name="pv_blnParameterRequired">Is the parameter required?</param>
        /// <param name="pv_strValidationExpression">Regular expression to validate parameter.</param>
        /// <param name="pv_strParameterHelpResource">Resource ID to use to build help for this parameter.</param>
        public ProcessParameterAttribute(
            string pv_strParameterName,
            string pv_strParameterAbbreviation,
            bool pv_blnParameterRequired,
            string pv_strValidationExpression,
            string pv_strParameterHelpResource)
        {
            PaytonByrd.GenericHelpers.ArgumentAssert.IsStringNotNullNorEmpty(pv_strParameterName, "pv_strParameterName");
            
            m_strParameterName = pv_strParameterName;
            m_strParameterAbbreviation = pv_strParameterAbbreviation;
            m_blnParameterRequired = pv_blnParameterRequired;
            m_strValidationExpression = pv_strValidationExpression;
            m_strParameterHelpResource = pv_strParameterHelpResource;
        }

        public static ParameterValidationError Validate(
            object pv_objInstance, 
            PropertyInfo pv_objPropertyInfo)
        {
            ArgumentAssert.IsGenericNotNull<object>(pv_objInstance, "pv_objInstance");
            ArgumentAssert.IsGenericNotNull<PropertyInfo>(pv_objPropertyInfo, "pv_objPropertyInfo");

            ParameterValidationError objResult = ParameterValidationError.CreateValid();

            object objValue = null;
            ProcessParameterAttribute objAttribute = null;

            try
            {
                List<ProcessParameterAttribute> objAttributes = 
                    new List<ProcessParameterAttribute>(
                        (ProcessParameterAttribute[])
                            pv_objPropertyInfo.GetCustomAttributes(
                                typeof(ProcessParameterAttribute), false));

                if (objAttributes.Count > 0)
                {
                    objAttribute = objAttributes[0];
                    objValue = pv_objPropertyInfo.GetValue(pv_objInstance, new object[0]);

                    if (objValue == null && objAttribute.m_blnParameterRequired)
                    {
                        return ParameterValidationError.CreateNotValid(string.Format(
                            Helpers.ResourceHelper.GetResource(CANNOT_BE_NULL_RESOURCE_ID),
                            pv_objPropertyInfo.Name));
                    }

                    if (objAttribute.m_strValidationExpression != ".*")
                    {
                        System.Text.RegularExpressions.Regex objRegex =
                            new System.Text.RegularExpressions.Regex(objAttribute.m_strValidationExpression);

                        if (!objRegex.IsMatch(objValue.ToString()))
                        {
                            return ParameterValidationError.CreateNotValid(string.Format(
                                Helpers.ResourceHelper.GetResource(DOES_NOT_MATCH_EXPRESSION_RESOURCE_ID),
                                pv_objPropertyInfo.Name));
                        }
                    }
                }
            }
            catch(Exception objException)
            {
                ParameterValidationError.CreateNotValid(objException.ToString());
            }

            return objResult;
        }
    }
}
