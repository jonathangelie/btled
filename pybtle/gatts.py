class gatts():

    def __init__(self):
        self.attrs = []
        self.attr = None
        self.service = None

    def addService(self, uuid):
        '''
        Attribute Type (0x2800 for Primary Service)
        Attribute Value (Service UUID)
        Attribute Permission (Read Only, No Authentication, No Authorization)
        '''
        if None != self.attr:
            self.attrs.append(self.attr)

        self.attr = {}
        self.attr["service"] = {}
        self.attr["service"]["uuid"] = uuid
        self.attr["service"]["chars"] = []

        self.service = {}
        self.service["prim_svc"] = uuid
        self.service["chars"] = []

    def addCharacteristic(self, uuid, value, properties = 1):
        '''
        Characteristic Declaration [M](Information about the characteristic)
            - Attribute Type (Characteristic UUID)
            - Attribute Value
                - Characteristic properties (bit field)
                - Characteristic value attribute handle
                - Characteristic value UUID
            -  Attribute permision (Read Only, No Authentication, No Authorization)
        Characteristic Value Declaration [M](Charactertistic Value)
        Characteristic Descriptor Declaration [O]: Additional information
        '''
        if None == self.attr:
            print "please addService before"
            return

        charac = {}
        charac["uuid"] = uuid
        if value != None:
            charac["value"] = value
        charac["properties"] = properties
        self.attr["service"]["chars"].append(charac)

    def _addService(self, uuid):
        '''
        Attribute Type (0x2800 for Primary Service)
        Attribute Value (Service UUID)
        Attribute Permission (Read Only, No Authentication, No Authorization)
        '''
        if None != self.service:
            self.attrs.append(self.service)

        self.service = {}
        self.service["prim_svc"] = uuid
        self.service["chars"] = []
        
    def _addCharacteristic(self, uuid, value, properties = 1):
        '''
        Characteristic Declaration [M](Information about the characteristic)
            - Attribute Type (Characteristic UUID)
            - Attribute Value
                - Characteristic properties (bit field)
                - Characteristic value attribute handle
                - Characteristic value UUID
            -  Attribute permision (Read Only, No Authentication, No Authorization)
        Characteristic Value Declaration [M](Charactertistic Value)
        Characteristic Descriptor Declaration [O]: Additional information
        '''
        if None == self.service:
            print "please addService before"
            return

        charac = {}
        charac["uuid"] = uuid
        if value != None:
            charac["value"] = value
        charac["properties"] = properties
        self.service["chars"].append(charac)

    def register(self):
        '''
        +-----------------------------------------------------+
        |                 Attribute Structure                 |
        +-----------------------------------------------------+
        |      2 bytes     |  2 or 16 bytes |  0 to 512 bytes |
        +------------------+----------------+-----------------+
        | Attribute handle | Attribute Type | Attribute value |
        +------------------+----------------+-----------------+
        '''
        #self.attrs.append(self.service)
        self.attrs.append(self.attr)
        self.service = None

g = gatts()
g.addService(0x180A)                 #Device Information
g.addCharacteristic(0x2A29, "Btled")        # Manufacturer Name
g.addCharacteristic(0x2A24, "Gatts")        # Model Number String
g.addCharacteristic(0x2A25, "1.0.0")        # Serial Number String
g.addService(0x180f)                 # BAttery Service
g.addCharacteristic(0x2A19, "50")           # Battery Level
g.register()

print g.attrs
print len(g.attrs)
for elt in g.attrs:

    print "service \n"
    print elt["service"]
    print "characteristic \n"
    print elt["service"]["chars"]