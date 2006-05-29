#include "xmlutil.h"

#include <iostream>

namespace xml {

// Return the NodeList resulting from an XPath expression.

NodeList findPath(Document doc, std::string path) throw (XPathException) {

    // Set up the XPath context
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    
    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        std::cerr << "ERROR: xml::findPath() failed to create XPath context "
                  << "when searching for " << path << std::endl;
        throw XPathException("Failed to create XPath context");
    }
    
    // Evaluate the expression
    
    const xmlChar* xpath = reinterpret_cast<const xmlChar*>(path.c_str());
    result = xmlXPathEvalExpression(xpath, context);
    if (result == NULL) {
        std::cerr << "ERROR: xml::findPath() failed to evaluate expression "
                  << path << std::endl;
        throw XPathException("Failed to evaluate XPath expression");
    }
    
    // Construct the return vector. This may be empty if the provided XPath
    // expression does not identify any nodes.
    
    NodeList retval;
    xmlNodeSetPtr nodeset = result->nodesetval;
    for (int i = 0; i < nodeset->nodeNr; i++) {
        retval.push_back(nodeset->nodeTab[i]);
    }
    return retval;   
}

} // namespace xml
