#!/usr/bin/env python


"""
 Copyright 2007-2017 United States Government as represented by the
 Administrator of The National Aeronautics and Space Administration.
 No copyright is claimed in the United States under Title 17, U.S. Code.
 All Rights Reserved.
"""

""" 
 @file validation_custom.py
 
 This file contains an example demonstrating how to implement additional
 Message validation logic in addition to that which the GMSEC API provides.

 Note: This example focuses on adding additional validation upon the receipt
 of a message.  It almost goes without saying that additional logic can be
 added to a program prior to invoking the publish() function without having
 to do anything special.
"""
import libgmsec_python
import sys 

PROD_MESSAGE_SUBJECT = "GMSEC.MISSION.SATELLITE.MSG.PROD.PRODUCT_MESSAGE"


# Create a callback and define message content validation logic which will
# be used in combination with the GMSEC API validation.
class ValidationCallback(libgmsec_python.ConnectionManagerCallback):

    def __init__(self):
        libgmsec_python.ConnectionManagerCallback.__init__(self)

    def onMessage(self, connMgr, message):
        try:
            # Run the message through the GMSEC API-supplied validation
            connMgr.getSpecification().validateMessage(message)

            # In this example scenario, we are expecting to receive a
            # GMSEC PROD message containing a URI to a location on the disk
            # where a product file has been placed for retrieval.  In this
            # case, we want to validate that the location on the disk is
            # in an area which we would expect (i.e. Something that the
            # team has agreed upon prior to operational deployment).
            #
            # By validating this URI, we ensure that no malicious users
            # have infiltrated the system and somehow modified the messages
            # to cause us to retrieve a file from an unknown location.

            # Start by checking to ensure that this is a PROD message
            if (isProdMsg(message)):
                prodMessage = libgmsec_python.ProductFileMessage(message.toXML())

                # Determine how many product files this message
                # contains
                numFiles = prodMessage.getNumProductFiles()

                # Extract the Product File URI location(s) from the
                # message
                #
                # Note: Not all Product Files will be transferred by
                # URI.  If implementing something like this, add some
                # error handling.
                for i in xrange(0,numFiles):
                                
                    # TODO (MIST Note): When using getProductFile(size_t), the number does not correspond to the number in the Field name.
                    # e.g. If the Message contains 1 ProductFile, using getProductFile(1) will return an error, whereas getProductFile(0) will not.
                    prodUri = prodMessage.getProductFile(i).getURI()

                    # Check to ensure that the URI contains "//hostname/dir"
                    if (prodUri.find("//hostname/dir") == -1):
                        
                        errorMsg = "Received an invalid PROD Message (bad URI):\n" + message.toXML()
                        raise libgmsec_python.Exception(libgmsec_python.MIST_ERROR, libgmsec_python.MESSAGE_FAILED_VALIDATION, errorMsg)
                             
                            
                libgmsec_python.logInfo("Received a valid message:\n" + message.toXML())
                        
        except libgmsec_python.Exception as e:
            libgmsec_python.logError(e.what())


def main():

    if(len(sys.argv) <= 1):
        usageMessage = "usage: " + sys.argv[0] + " mw-id=<middleware ID>"
        print usageMessage
        return -1


    # Load the command-line input into a GMSEC Config object
    config = libgmsec_python.Config()

    for arg in sys.argv[1:]:
        value = arg.split('=')
        config.addValue(value[0], value[1])


    # If it was not specified in the command-line arguments, set LOGLEVEL
    # to 'INFO' and LOGFILE to 'stdout' to allow the program report output
    # on the terminal/command line

    initializeLogging(config);

    # Enable Message validation.  This parameter is "false" by default.
    config.addValue("GMSEC-MSG-CONTENT-VALIDATE", "true")

    libgmsec_python.logInfo(libgmsec_python.ConnectionManager.getAPIVersion())

    try:
        connMgr = libgmsec_python.ConnectionManager(config)

        libgmsec_python.logInfo("Opening the connection to the middleware server")
        connMgr.initialize()

        connMgr.getLibraryVersion()

        # Set up the ValidationCallback and subscribe
        vc = ValidationCallback()
        connMgr.subscribe(PROD_MESSAGE_SUBJECT, vc)

        # Start the AutoDispatcher
        connMgr.startAutoDispatch()

        # Create and publish a simple Product File Message
        setupStandardFields(connMgr)

        productMessage = ProductFileMessage = createProductFileMessage(connMgr, "//hostname/dir/filename")

        # Publish the message to the middleware bus
        connMgr.publish(productMessage)

        productMessage = createProductFileMessage(connMgr, "//badhost/dir/filename")

        connMgr.publish(productMessage)

        # Disconnect from the middleware and clean up the Connection
        connMgr.cleanup()
        
    except libgmsec_python.Exception as e:
        libgmsec_python.logError(e.what())
        return -1

    return 0


def initializeLogging(config):

    logLevel  = config.getValue("LOGLEVEL")
    logFile   = config.getValue("LOGFILE")

    if (not logLevel):

        config.addValue("LOGLEVEL", "INFO")

    if (not logFile):

        config.addValue("LOGFILE", "STDERR")


def setupStandardFields(connMgr):

    definedFields = libgmsec_python.FieldList()

    missionField = libgmsec_python.StringField("MISSION-ID", "MISSION")
    satIdField = libgmsec_python.StringField("SAT-ID-PHYSICAL", "SPACECRAFT")
    facilityField = libgmsec_python.StringField("FACILITY", "GMSEC Lab")
    componentField = libgmsec_python.StringField("COMPONENT", "validation_custom")

    definedFields.append(missionField)
    definedFields.append(satIdField)
    definedFields.append(facilityField)
    definedFields.append(componentField)

    connMgr.setStandardFields(definedFields)


def createProductFileMessage(connMgr, filePath):

    externalFile = libgmsec_python.ProductFile("External File", "External File Description", "1.0.0", "TXT", filePath)

    productMessage = libgmsec_python.ProductFileMessage(PROD_MESSAGE_SUBJECT, libgmsec_python.ResponseStatus.SUCCESSFUL_COMPLETION, libgmsec_python.Message.PUBLISH, "AUTO", "DM", connMgr.getSpecification())
    productMessage.addProductFile(externalFile)

    connMgr.addStandardFields(productMessage)

    return productMessage
    


def isProdMsg(message):

    result = False
    result = (message.getStringValue("MESSAGE-TYPE") == "MSG" and message.getStringValue("MESSAGE-SUBTYPE") == "PROD") 

    return result



#
# Main entry point of script
#
if __name__=="__main__":
    sys.exit(main())


