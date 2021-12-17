using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Text;
using System.Collections.Generic;
namespace PaytonByrd_GenericHelpersTests
{
    /// <summary>
    ///This is a test class for PaytonByrd.GenericHelpers.ArgumentAssert and is intended
    ///to contain all PaytonByrd.GenericHelpers.ArgumentAssert Unit Tests
    ///</summary>
    [TestClass()]
    public class ArgumentAssertTest
    {
        #region Test Context
        private TestContext testContextInstance;

        /// <summary>
        ///Gets or sets the test context which provides
        ///information about and functionality for the current test run.
        ///</summary>
        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }
        #endregion Test Context

        #region Additional test attributes
        // 
        //You can use the following additional attributes as you write your tests:
        //
        //Use ClassInitialize to run code before running the first test in the class
        //
        //[ClassInitialize()]
        //public static void MyClassInitialize(TestContext testContext)
        //{
        //}
        //
        //Use ClassCleanup to run code after all tests in a class have run
        //
        //[ClassCleanup()]
        //public static void MyClassCleanup()
        //{
        //}
        //
        //Use TestInitialize to run code before running each test
        //
        //[TestInitialize()]
        //public void MyTestInitialize()
        //{
        //}
        //
        //Use TestCleanup to run code after each test has run
        //
        //[TestCleanup()]
        //public void MyTestCleanup()
        //{
        //}
        //
        #endregion

        #region IsGenericInRange
        #region Sunny Day
        #region Class
        /// <summary>
        ///A test for IsGenericInRange&lt;&gt; (T, string, T, T)
        ///</summary>
        [TestMethod()]
        public void IsGenericInRangeTest_SunnyDay_Class()
        {
            string pv_tValue = "A";

            string pv_strName = "pv_strName";

            string pv_tLow = "A";

            string pv_tHigh = "A";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericInRange(pv_tValue, pv_strName, pv_tLow, pv_tHigh);
        }
        #endregion Class

        #region Nullable
        /// <summary>
        ///A test for IsGenericInRange&lt;&gt; (T, string, T, T)
        ///</summary>
        [TestMethod()]
        public void IsGenericInRangeTest_SunnyDay_Nullable()
        {
            DateTime? dtmNow = DateTime.Today;

            DateTime? pv_tValue = dtmNow;

            string pv_strName = "pv_strName";

            DateTime? pv_tLow = dtmNow;

            DateTime? pv_tHigh = dtmNow;

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericInRange(pv_tValue, pv_strName, pv_tLow, pv_tHigh);
        }
        #endregion Nullable

        #region Struct
        /// <summary>
        ///A test for IsGenericInRange&lt;&gt; (T, string, T, T)
        ///</summary>
        [TestMethod()]
        public void IsGenericInRangeTest_SunnyDay_Struct()
        {
            DateTime dtmNow = DateTime.Today;

            DateTime pv_tValue = dtmNow;

            string pv_strName = "pv_strName";

            DateTime pv_tLow = dtmNow;

            DateTime pv_tHigh = dtmNow;

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericInRange(pv_tValue, pv_strName, pv_tLow, pv_tHigh);
        }
        #endregion Struct
        #endregion Sunny Day

        #region Accuracy
        #region Class
        /// <summary>
        ///A test for IsGenericInRange&lt;&gt; (T, string, T, T)
        ///</summary>
        [TestMethod()]
        [ExpectedException(typeof(ArgumentNullException))]
        public void IsGenericInRangeTest_Accuracy_Class()
        {
            string pv_tValue = null;

            string pv_strName = "pv_strName";

            string pv_tLow = null;

            string pv_tHigh = null;

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericInRange(pv_tValue, pv_strName, pv_tLow, pv_tHigh);

            Assert.Fail("Expected ArgumentNullException");
        }
        #endregion Class

        #region Nullable
        /// <summary>
        ///A test for IsGenericInRange&lt;&gt; (T, string, T, T)
        ///</summary>
        [TestMethod()]
        [ExpectedException(typeof(ArgumentNullException))]
        public void IsGenericInRangeTest_Accuracy_Nullable()
        {
            DateTime? dtmNow = null;

            DateTime? pv_tValue = dtmNow;

            string pv_strName = "pv_strName";

            DateTime? pv_tLow = dtmNow;

            DateTime? pv_tHigh = dtmNow;

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericInRange(pv_tValue, pv_strName, pv_tLow, pv_tHigh);

            Assert.Fail("Expected ArgumentNullException");
        }
        #endregion Nullable
        #endregion Accuracy

        #endregion IsGenericInRange

        #region IsGenericNotNull
        #region Sunny Day
        #region Class
        /// <summary>
        ///A test for IsGenericNotNull&lt;&gt; (T, string)
        ///</summary>
        [TestMethod()]
        public void IsGenericNotNullTest_SunnyDay_Class()
        {
            string pv_tValue = "Value";
             
            string pv_strName = "pv_strName";
             
            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericNotNull<object>(pv_tValue, pv_strName);
        }
        #endregion Class

        #region Nullable
        /// <summary>
        ///A test for IsGenericNotNull&lt;&gt; (T, string)
        ///</summary>
        [TestMethod()]
        public void IsGenericNotNullTest_SunnyDay_Nullable()
        {
            int? pv_tValue = 10;

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericNotNull<int?>(pv_tValue, pv_strName);
        }
        #endregion Nullable

        #region Struct
        /// <summary>
        ///A test for IsGenericNotNull&lt;&gt; (T, string)
        ///</summary>
        [TestMethod()]
        public void IsGenericNotNullTest_SunnyDay_Struct()
        {
            int pv_tValue = 10;

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericNotNull<int>(pv_tValue, pv_strName);
        }
        #endregion Struct
        #endregion Sunny Day

        #region Accuracy
        #region Class
        /// <summary>
        ///A test for IsGenericNotNull&lt;&gt; (T, string)
        ///</summary>
        [TestMethod()]
        [ExpectedException(typeof(ArgumentNullException))]
        public void IsGenericNotNullTest_Accuracy_Class()
        {
            string pv_tValue = null;

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericNotNull<object>(pv_tValue, pv_strName);
            
            Assert.Fail("Expected ArgumentNullException.");
        }
        #endregion Class

        #region Nullable
        /// <summary>
        ///A test for IsGenericNotNull&lt;&gt; (T, string)
        ///</summary>
        [TestMethod()]
        [ExpectedException(typeof(ArgumentNullException))]
        public void IsGenericNotNullTest_Accuracy_Nullable()
        {
            int? pv_tValue = null;

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsGenericNotNull<int?>(pv_tValue, pv_strName);

            Assert.Fail("Expected ArgumentNullException.");
        }
        #endregion Nullable
        #endregion Accuracy

        #endregion IsGenericNotNull

        #region IsInRange
        #region Sunny Day
        /// <summary>
        ///A test for IsInRange (object, string, object, object)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        public void IsInRangeTest_SunnyDay()
        {
            object pv_objValue = 5;

            string pv_strName = null;

            object pv_objLow = 5;

            object pv_objHigh = 5;

            PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsInRange(pv_objValue, pv_strName, pv_objLow, pv_objHigh);
        }
        #endregion Sunny Day

        #region Accuracy
        #region Null Value
        /// <summary>
        ///A test for IsInRange (object, string, object, object)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        [ExpectedException(typeof(ArgumentException), "Argument does not implement IComparable.")]
        public void IsInRangeTest_Accuracy_NullValue()
        {
            object pv_objValue = null;

            string pv_strName = null;

            object pv_objLow = null;

            object pv_objHigh = null;

            try
            {
                PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsInRange(pv_objValue, pv_strName, pv_objLow, pv_objHigh);
            }
            catch (System.Reflection.TargetInvocationException objException)
            {
                throw objException.InnerException;
            }

            Assert.Fail("Expected ArgumentNullExcpetion.");
        }
        #endregion Null Value

        #region Not Comparable
        /// <summary>
        ///A test for IsInRange (object, string, object, object)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        [ExpectedException(typeof(ArgumentException), "Argument does not implement IComparable.")]
        public void IsInRangeTest_Accuracy_NotComparable()
        {
            object pv_objValue = new object();

            string pv_strName = null;

            object pv_objLow = new object();

            object pv_objHigh = new object();

            try
            {
                PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsInRange(pv_objValue, pv_strName, pv_objLow, pv_objHigh);
            }
            catch (System.Reflection.TargetInvocationException objException)
            {
                throw objException.InnerException;
            }

            Assert.Fail("Expected ArgumentNullExcpetion.");
        }
        #endregion Not Comparable

        #region Less Than
        /// <summary>
        ///A test for IsInRange (object, string, object, object)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        [ExpectedException(typeof(ArgumentOutOfRangeException))]
        public void IsInRangeTest_Accuracy_LessThan()
        {
            object pv_objValue = 0;

            string pv_strName = null;

            object pv_objLow = 5;

            object pv_objHigh = 5;

            try
            {
                PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsInRange(pv_objValue, pv_strName, pv_objLow, pv_objHigh);
            }
            catch (System.Reflection.TargetInvocationException objException)
            {
                throw objException.InnerException;
            }

            Assert.Fail("Expected ArgumentNullExcpetion.");
        }
        #endregion Less Than

        #region Greater Than
        /// <summary>
        ///A test for IsInRange (object, string, object, object)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        [ExpectedException(typeof(ArgumentOutOfRangeException))]
        public void IsInRangeTest_Accuracy_GreaterThan()
        {
            object pv_objValue = 10;

            string pv_strName = null;

            object pv_objLow = 5;

            object pv_objHigh = 5;

            try
            {
                PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsInRange(pv_objValue, pv_strName, pv_objLow, pv_objHigh);
            }
            catch (System.Reflection.TargetInvocationException objException)
            {
                throw objException.InnerException;
            }

            Assert.Fail("Expected ArgumentNullExcpetion.");
        }
        #endregion Greater Than
        #endregion Accuracy

        #endregion IsInRange

        #region IsNotNull
        #region Sunny Day
        /// <summary>
        ///A test for IsNotNull (object, string)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        public void IsNotNullTest_SunnyDay()
        {
            object pv_objValue = new object();

            string pv_strName = "pv_strName";

            try
            {
                PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsNotNull(pv_objValue, pv_strName);
            }
            catch (System.Reflection.TargetInvocationException objException)
            {
                throw objException.InnerException;
            }
        }
        #endregion Sunny Day

        #region Accuracy Tests
        #region Is Null
        /// <summary>
        ///A test for IsNotNull (object, string)
        ///</summary>
        [DeploymentItem("PaytonByrd.GenericHelpers.dll")]
        [TestMethod()]
        [ExpectedException(typeof(ArgumentNullException))]
        public void IsNotNullTest_Accuracy_Null()
        {
            object pv_objValue = null;

            string pv_strName = "pv_strName";

            try
            {
                PaytonByrd_GenericHelpersTests.PaytonByrd_GenericHelpers_ArgumentAssertAccessor.IsNotNull(pv_objValue, pv_strName);
            }
            catch (System.Reflection.TargetInvocationException objException)
            {
                throw objException.InnerException;
            }

            Assert.Fail("Expected ArgumentNullException.");
        }
        #endregion Is Null
        #endregion Accuracy Tests

        #endregion IsNotNull

        #region IsStringNotNullNorEmpty
        #region Sunny Day
        /// <summary>
        ///A test for IsStringNotNullNorEmpty (string, string)
        ///</summary>
        [TestMethod()]
        public void IsStringNotNullNorEmptyTest_SunnyDay()
        {
            string pv_strValue = "pv_strValue";

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsStringNotNullNorEmpty(pv_strValue, pv_strName);
        }
        #endregion Sunny Day

        #region Accuracy Tests
        #region NullString
        /// <summary>
        ///A test for IsStringNotNullNorEmpty (string, string)
        ///</summary>
        [TestMethod()]
        [ExpectedException(typeof(ArgumentNullException))]
        public void IsStringNotNullNorEmptyTest_Accuracy_NullString()
        {
            string pv_strValue = null;

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsStringNotNullNorEmpty(pv_strValue, pv_strName);

            Assert.Fail("Should have thrown ArgumentNullException.");
        }
        #endregion NullString


        #region EmptyString
        /// <summary>
        ///A test for IsStringNotNullNorEmpty (string, string)
        ///</summary>
        [TestMethod()]
        [ExpectedException(typeof(ArgumentException))]
        public void IsStringNotNullNorEmptyTest_Accuracy_EmptyString()
        {
            string pv_strValue = string.Empty;

            string pv_strName = "pv_strName";

            PaytonByrd.GenericHelpers.ArgumentAssert.IsStringNotNullNorEmpty(pv_strValue, pv_strName);

            Assert.Fail("Should have thrown ArgumentException.");
        }
        #endregion EmptyString
        #endregion Accuracy Tests
        #endregion IsStringNotNullNorEmpty

    }


}
