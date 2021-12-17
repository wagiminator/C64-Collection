using System;
using System.Collections.Generic;
using System.Text;

namespace PaytonByrd.ProcessConfiguration
{
    public class ParameterValidationError
    {
        public bool IsValid = false;
        public string Description = null;

        private ParameterValidationError()
        {
            IsValid = true;
            Description = null;
        }

        public static ParameterValidationError CreateValid()
        {
            return new ParameterValidationError();
        }

        public static ParameterValidationError CreateNotValid(string pv_strDescription)
        {
            ParameterValidationError objResult = new ParameterValidationError();

            objResult.IsValid = false;
            objResult.Description = pv_strDescription;

            return objResult;
        }
    }
}
