#ifndef _RENDEZVOUS_SERVER_ROOT_CERTIFICATE_H
#define _RENDEZVOUS_SERVER_ROOT_CERTIFICATE_H
/**
 * @file
 * This file defines the Rendezvous Server specific certificate authority
 * file contents in PEM format.
 */

/******************************************************************************
 * Copyright 2012, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 ******************************************************************************/

#include "Status.h"

#define QCC_MODULE "RENDEZVOUS_SERVER_ROOT_CERTIFICATE"

namespace qcc {

static const char* RendezvousServerRootCertificate;

static const char RendezvousTestServerRootCertificate[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEVzCCAz+gAwIBAgIQFoFkpCjKEt+rEvGfsbk1VDANBgkqhkiG9w0BAQUFADCB\n"
    "jDELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTAwLgYDVQQL\n"
    "EydGb3IgVGVzdCBQdXJwb3NlcyBPbmx5LiAgTm8gYXNzdXJhbmNlcy4xMjAwBgNV\n"
    "BAMTKVZlcmlTaWduIFRyaWFsIFNlY3VyZSBTZXJ2ZXIgUm9vdCBDQSAtIEcyMB4X\n"
    "DTA5MDQwMTAwMDAwMFoXDTI5MDMzMTIzNTk1OVowgYwxCzAJBgNVBAYTAlVTMRcw\n"
    "FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjEwMC4GA1UECxMnRm9yIFRlc3QgUHVycG9z\n"
    "ZXMgT25seS4gIE5vIGFzc3VyYW5jZXMuMTIwMAYDVQQDEylWZXJpU2lnbiBUcmlh\n"
    "bCBTZWN1cmUgU2VydmVyIFJvb3QgQ0EgLSBHMjCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    "ggEPADCCAQoCggEBAMCJggWnSVAcIomnvCFhXlCdgafCKCDxVSNQY2jhYGZXcZsq\n"
    "ToJmDQ7b9JO39VCPnXELOENP2+4FNCUQnzarLfghsJ8kQ9pxjRTfcMp0bsH+Gk/1\n"
    "qLDgvf9WuiBa5SM/jXNvroEQZwPuMZg4r2E2k0412VTq9ColODYNDZw3ziiYdSjV\n"
    "fY3VfbsLSXJIh2jaJC5kVRsUsx72s4/wgGXbb+P/XKr15nMIB0yH9A5tiCCXQ5nO\n"
    "EV7/ddZqmL3zdeAtyGmijOxjwiy+GS6xr7KACfbPEJYZYaS/P0wctIOyQy6CkNKL\n"
    "o5vDDkOZks0zjf6RAzNXZndvsXEJpQe5WO1avm8CAwEAAaOBsjCBrzAPBgNVHRMB\n"
    "Af8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkw\n"
    "VzBVFglpbWFnZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZ\n"
    "LjAlFiNodHRwOi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjAdBgNVHQ4E\n"
    "FgQUSBnnkm+SnTRjmcDwmcjWpYyMf2UwDQYJKoZIhvcNAQEFBQADggEBADuswa8C\n"
    "0hunHp17KJQ0WwNRQCp8f/u4L8Hz/TiGfybnaMXgn0sKI8Xe79iGE91M7vrzh0Gt\n"
    "ap0GLShkiqHGsHkIxBcVMFbEQ1VS63XhTeg36cWQ1EjOHmu+8tQe0oZuwFsYYdfs\n"
    "n4EZcpspiep9LFc/hu4FE8SsY6MiasHR2Ay97UsC9A3S7ZaoHfdwyhtcINXCu2lX\n"
    "W0Gpi3vzWRvwqgua6dm2WVKJfvPfmS1mAP0YmTcIwjdiNXiU6sSsJEoNlTR9zCoo\n"
    "4oKQ8wVoWZpbuPZb5geszhS7YsABUPIAAfF1YQCiMULtpa6HFzzm7sdf72N3HfwE\n"
    "aQNg95KnKGrrDUI=\n"
    "-----END CERTIFICATE-----"
};

static const char RendezvousDeploymentServerRootCertificate[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF7DCCBNSgAwIBAgIQbsx6pacDIAm4zrz06VLUkTANBgkqhkiG9w0BAQUFADCB\n"
    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
    "aG9yaXR5IC0gRzUwHhcNMTAwMjA4MDAwMDAwWhcNMjAwMjA3MjM1OTU5WjCBtTEL\n"
    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQg\n"
    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykxMDEvMC0GA1UEAxMmVmVy\n"
    "aVNpZ24gQ2xhc3MgMyBTZWN1cmUgU2VydmVyIENBIC0gRzMwggEiMA0GCSqGSIb3\n"
    "DQEBAQUAA4IBDwAwggEKAoIBAQCxh4QfwgxF9byrJZenraI+nLr2wTm4i8rCrFbG\n"
    "5btljkRPTc5v7QlK1K9OEJxoiy6Ve4mbE8riNDTB81vzSXtig0iBdNGIeGwCU/m8\n"
    "f0MmV1gzgzszChew0E6RJK2GfWQS3HRKNKEdCuqWHQsV/KNLO85jiND4LQyUhhDK\n"
    "tpo9yus3nABINYYpUHjoRWPNGUFP9ZXse5jUxHGzUL4os4+guVOc9cosI6n9FAbo\n"
    "GLSa6Dxugf3kzTU2s1HTaewSulZub5tXxYsU5w7HnO1KVGrJTcW/EbGuHGeBy0RV\n"
    "M5l/JJs/U0V/hhrzPPptf4H1uErT9YU3HLWm0AnkGHs4TvoPAgMBAAGjggHfMIIB\n"
    "2zA0BggrBgEFBQcBAQQoMCYwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLnZlcmlz\n"
    "aWduLmNvbTASBgNVHRMBAf8ECDAGAQH/AgEAMHAGA1UdIARpMGcwZQYLYIZIAYb4\n"
    "RQEHFwMwVjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2Nw\n"
    "czAqBggrBgEFBQcCAjAeGhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMDQG\n"
    "A1UdHwQtMCswKaAnoCWGI2h0dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMtZzUu\n"
    "Y3JsMA4GA1UdDwEB/wQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglp\n"
    "bWFnZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNo\n"
    "dHRwOi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjAoBgNVHREEITAfpB0w\n"
    "GzEZMBcGA1UEAxMQVmVyaVNpZ25NUEtJLTItNjAdBgNVHQ4EFgQUDURcFlNEwYJ+\n"
    "HSCrJfQBY9i+eaUwHwYDVR0jBBgwFoAUf9Nlp8Ld7LvwMAnzQzn6Aq8zMTMwDQYJ\n"
    "KoZIhvcNAQEFBQADggEBAAyDJO/dwwzZWJz+NrbrioBL0aP3nfPMU++CnqOh5pfB\n"
    "WJ11bOAdG0z60cEtBcDqbrIicFXZIDNAMwfCZYP6j0M3m+oOmmxw7vacgDvZN/R6\n"
    "bezQGH1JSsqZxxkoor7YdyT3hSaGbYcFQEFn0Sc67dxIHSLNCwuLvPSxe/20majp\n"
    "dirhGi2HbnTTiN0eIsbfFrYrghQKlFzyUOyvzv9iNw2tZdMGQVPtAhTItVgooazg\n"
    "W+yzf5VK+wPIrSbb5mZ4EkrZn0L74ZjmQoObj49nJOhhGbXdzbULJgWOw27EyHW4\n"
    "Rs/iGAZeqa6ogZpHFt4MKGwlJ7net4RYxh84HqTEy2Y=\n"
    "-----END CERTIFICATE-----"
};

static QStatus InitializeServerRootCertificate(String Server)
{
    QStatus status = ER_OK;

    if (Server == String("rdvs-test.qualcomm.com")) {
        RendezvousServerRootCertificate = RendezvousTestServerRootCertificate;
    } else if (Server == String("rdvs.qualcomm.com")) {
        RendezvousServerRootCertificate = RendezvousDeploymentServerRootCertificate;
    } else {
        status = ER_RENDEZVOUS_SERVER_ROOT_CERTIFICATE_UNINITIALIZED;
    }
    return status;
}


} // namespace qcc

#undef QCC_MODULE

#endif // _RENDEZVOUS_SERVER_ROOT_CERTIFICATE_H
