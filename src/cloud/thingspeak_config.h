#ifndef GREENHOUSE_THINGSPEAK_CONFIG_H
#define GREENHOUSE_THINGSPEAK_CONFIG_H

// -----------------------------------------------------------------------------
// ThingSpeak Configuration:
// These definitions configure the connection to ThingSpeak for uploading sensor data
// and receiving TalkBack commands. The TalkBackID and API keys are used to authenticate
// and direct data to the appropriate channel on ThingSpeak.
// -----------------------------------------------------------------------------
#define TalkBackID 54160  // Unique identifier for TalkBack commands specific to this system

// API key to write data to ThingSpeak. This key allows HTTP POST updates.
#define THINGSPEAK_WRITE_API_KEY "6V2KTU7QP0DRRB88"

// API key for TalkBack commands on ThingSpeak. This key allows reception of remote commands.
#define THINGSPEAK_TALKBACK_API_KEY "SYS3WRA4JERPF0M8"

// URL endpoint for retrieving the last TalkBack command from ThingSpeak.
#define THINGSPEAK_TALKBACK_URL "/talkbacks/54160/commands/last.json"

// -----------------------------------------------------------------------------
// TLS Certificate Configuration for Server Authentication:
// The TLS_JOES_SERVER define contains the PEM-encoded certificate required
// to authenticate the server during the TLS handshake. This certificate is used
// to ensure secure communication when the device connects to the ThingSpeak server.
// -----------------------------------------------------------------------------
#define TLS_JOES_SERVER "-----BEGIN CERTIFICATE-----\r\n"\
"MIIDXTCCAkWgAwIBAgIUPp2A/waMZh3cPqpQ9xqhu5d3lA0wDQYJKoZIhvcNAQEL\r\n"\
"BQAwPjEXMBUGA1UEAwwOMTguMTk4LjE4OC4xNTExCzAJBgNVBAYTAlVTMRYwFAYD\r\n"\
"VQQHDA1TYW4gRnJhbnNpc2NvMB4XDTI0MDYxMjExMjEwM1oXDTI1MDYwMzExMjEw\r\n"\
"M1owPjEXMBUGA1UEAwwOMTguMTk4LjE4OC4xNTExCzAJBgNVBAYTAlVTMRYwFAYD\r\n"\
"VQQHDA1TYW4gRnJhbnNpc2NvMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\r\n"\
"AQEAuWejVUsk/cYJHp+vOYkBzWdSvHlYWbkdWf2HnHy8qYLMJ/sQyYcL9XEv85dq\r\n"\
"HrOCuS1vp7UC0YxnfFQ2tmQ9PNqaEUOOvIwJUOK5jutK+H16gFTbOHM4EdcY1WkJ\r\n"\
"43jffHSiq7RRiAUhTwh+2ISCMAxPlXcOiEPoUrFauOKTRMvBFcfgqFHbOdCA9X5z\r\n"\
"ol0JzdeV9MMYtSWhMi+F+DJBMrNDxQhymJFyt6p9ft0v8m5B5mTKGuhppMCUSHNP\r\n"\
"ij3WQkTnByOynUAQ3WG/LaSNg1ItqPVf9/RHKWWViRAwB4DEfOoeKkM2EFHqxHLw\r\n"\
"bjybmleFnxQguzX8+oEe9NKGTQIDAQABo1MwUTAdBgNVHQ4EFgQUx8JPYn//MjiT\r\n"\
"4o38VAS4advRrtQwHwYDVR0jBBgwFoAUx8JPYn//MjiT4o38VAS4advRrtQwDwYD\r\n"\
"VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEADqaV5F+HhRqL35O5sycZ\r\n"\
"E4Gn8KewK/BDWkmQzqP5+w1PZ9bUGiSY49ftT2EGiLoOawnztkGBWX1Sv520T5qE\r\n"\
"wvB/rDzxOU/v9OIUTqCX7X68zVoN7A7r1iP6o66mnfgu9xDSk0ROZ73bYtaWL/Qq\r\n"\
"SJWBN1pPY2ekFxYNwBg8C1DTJ3H51H6R7kN0wze7lMN1tglrvLl1e60a8rm+QNwX\r\n"\
"FzQGTenLecgMGeXVsIGhnivQTvF2HN+EcXHs8O8LzHpX7fpt/KcsBx+kYmltkdJW\r\n"\
"QaFXAdvGJkhKEwJVn3qETVlTdtSKpc/1KdXq/01HuX7cPfXVMGJVXuJAk6Yxgx8z\r\n"\
"Ew==\r\n"\
"-----END CERTIFICATE-----\r\n"

#endif //GREENHOUSE_THINGSPEAK_CONFIG_H