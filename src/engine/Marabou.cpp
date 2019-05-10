/*********************                                                        */
/*! \file Marabou.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "AcasParser.h"
#include "File.h"
#include "MStringf.h"
#include "Marabou.h"
#include "Options.h"
#include "PropertyParser.h"
#include "ReluplexError.h"

Marabou::Marabou()
    : _acasParser( NULL )
{
}

Marabou::~Marabou()
{
    if ( _acasParser )
    {
        delete _acasParser;
        _acasParser = NULL;
    }
}

void Marabou::run( int argc, char **argv )
{
    struct timespec start = TimeUtils::sampleMicro();

    Options *options = Options::get();
    options->parseOptions( argc, argv );

    prepareInputQuery();
    solveQuery();

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );
}

void Marabou::prepareInputQuery()
{
    /*
      Step 1: extract the network
    */
    String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );
    if ( !File::exists( networkFilePath ) )
    {
        printf( "Error: the specified network file (%s) doesn't exist!\n", networkFilePath.ascii() );
        throw ReluplexError( ReluplexError::FILE_DOESNT_EXIST, networkFilePath.ascii() );
    }
    printf( "Network: %s\n", networkFilePath.ascii() );

    // For now, assume the network is given in ACAS format
    _acasParser = new AcasParser( networkFilePath );
    _acasParser->generateQuery( _inputQuery );

    /*
      Step 2: extract the property in question
    */
    String propertyFilePath = Options::get()->getString( Options::PROPERTY_FILE_PATH );
    if ( propertyFilePath != "" )
    {
        printf( "Property: %s\n", propertyFilePath.ascii() );
        PropertyParser().parse( propertyFilePath, _inputQuery );
    }
    else
        printf( "Property: None\n" );

    printf( "\n" );



    // _inputQuery.storeDebuggingSolution(0, 0.6798577687);
    // _inputQuery.storeDebuggingSolution(1, 0.0);
    // _inputQuery.storeDebuggingSolution(2, 0.25);
    // _inputQuery.storeDebuggingSolution(3, 0.45);
    // _inputQuery.storeDebuggingSolution(4, -0.46571037502939155);
    // double inputs [5] = { 0.6798577687, 0.0, 0.25, 0.45, -0.46571037502939155 };
    // double outputs [5];
    // std::cout << "Output: ";
    // _inputQuery.getNetworkLevelReasoner()->evaluate(inputs, outputs);
    // for (unsigned i = 0; i < 5; i++){
    //     std::cout << outputs[i] << " ";
    // }
    // std::cout << std::endl;

    // _inputQuery.setLowerBound(0, 0.6798577687);
    // _inputQuery.setLowerBound(1, 0.0);
    // _inputQuery.setLowerBound(2, 0.25);
    // _inputQuery.setLowerBound(3, 0.45);
    // _inputQuery.setLowerBound(4, -0.46571037502939155);

    // _inputQuery.setUpperBound(0, 0.6798577687);
    // _inputQuery.setUpperBound(1, 0.0);
    // _inputQuery.setUpperBound(2, 0.25);
    // _inputQuery.setUpperBound(3, 0.45);
    // _inputQuery.setUpperBound(4, -0.46571037502939155);
	_inputQuery.storeDebuggingSolution( 0, 0.679857768700000 );
	_inputQuery.storeDebuggingSolution( 1, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 2, 0.250000000000000 );
	_inputQuery.storeDebuggingSolution( 3, 0.450000000000000 );
	_inputQuery.storeDebuggingSolution( 4, -0.46571037502939155 );
    /*
      _inputQuery.storeDebuggingSolution( 5, -0.908931819797414 );
	_inputQuery.storeDebuggingSolution( 6, 0.875847303606065 );
	_inputQuery.storeDebuggingSolution( 7, 0.520161178550741 );
	_inputQuery.storeDebuggingSolution( 8, -0.303793536350260 );
	_inputQuery.storeDebuggingSolution( 9, -0.343259931230769 );
	_inputQuery.storeDebuggingSolution( 10, 0.119485925298914 );
	_inputQuery.storeDebuggingSolution( 11, 0.386475848235596 );
	_inputQuery.storeDebuggingSolution( 12, -0.879101511966137 );
	_inputQuery.storeDebuggingSolution( 13, -0.683934008948381 );
	_inputQuery.storeDebuggingSolution( 14, -0.811124833687713 );
	_inputQuery.storeDebuggingSolution( 15, 0.539427529702487 );
	_inputQuery.storeDebuggingSolution( 16, 0.604806084637551 );
	_inputQuery.storeDebuggingSolution( 17, -1.714288918089134 );
	_inputQuery.storeDebuggingSolution( 18, -0.651445782494790 );
	_inputQuery.storeDebuggingSolution( 19, -0.201399153675897 );
	_inputQuery.storeDebuggingSolution( 20, 0.586885487448293 );
	_inputQuery.storeDebuggingSolution( 21, -0.118458209166159 );
	_inputQuery.storeDebuggingSolution( 22, -0.923375663715775 );
	_inputQuery.storeDebuggingSolution( 23, 0.929427224390576 );
	_inputQuery.storeDebuggingSolution( 24, -0.207546797040248 );
	_inputQuery.storeDebuggingSolution( 25, -0.958749814461102 );
	_inputQuery.storeDebuggingSolution( 26, -1.169834543881958 );
	_inputQuery.storeDebuggingSolution( 27, -0.109381927770849 );
	_inputQuery.storeDebuggingSolution( 28, -1.759363648559353 );
	_inputQuery.storeDebuggingSolution( 29, 0.152049505785323 );
	_inputQuery.storeDebuggingSolution( 30, 0.180355112274895 );
	_inputQuery.storeDebuggingSolution( 31, 0.749208490962057 );
	_inputQuery.storeDebuggingSolution( 32, 0.018891219869575 );
	_inputQuery.storeDebuggingSolution( 33, -0.040689333437914 );
	_inputQuery.storeDebuggingSolution( 34, -0.563671822251094 );
	_inputQuery.storeDebuggingSolution( 35, -0.896871121907842 );
	_inputQuery.storeDebuggingSolution( 36, -1.416850794957569 );
	_inputQuery.storeDebuggingSolution( 37, -0.372948274477837 );
	_inputQuery.storeDebuggingSolution( 38, 0.504430669661189 );
	_inputQuery.storeDebuggingSolution( 39, -0.244362518192115 );
	_inputQuery.storeDebuggingSolution( 40, 0.080664708103537 );
	_inputQuery.storeDebuggingSolution( 41, 0.024257680041788 );
	_inputQuery.storeDebuggingSolution( 42, 0.133771175859501 );
	_inputQuery.storeDebuggingSolution( 43, -0.902886631137848 );
	_inputQuery.storeDebuggingSolution( 44, -1.152771411236718 );
	_inputQuery.storeDebuggingSolution( 45, 0.149514899259178 );
	_inputQuery.storeDebuggingSolution( 46, -1.335374858015083 );
	_inputQuery.storeDebuggingSolution( 47, -1.455409358272948 );
	_inputQuery.storeDebuggingSolution( 48, 0.228269589700240 );
	_inputQuery.storeDebuggingSolution( 49, 0.021731105194653 );
	_inputQuery.storeDebuggingSolution( 50, -1.603769164841789 );
	_inputQuery.storeDebuggingSolution( 51, -0.031780576294456 );
	_inputQuery.storeDebuggingSolution( 52, -0.239999480430785 );
	_inputQuery.storeDebuggingSolution( 53, 0.273974269681440 );
	_inputQuery.storeDebuggingSolution( 54, -1.196708775683364 );
	_inputQuery.storeDebuggingSolution( 55, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 56, 0.875847303606065 );
	_inputQuery.storeDebuggingSolution( 57, 0.520161178550741 );
	_inputQuery.storeDebuggingSolution( 58, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 59, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 60, 0.119485925298914 );
	_inputQuery.storeDebuggingSolution( 61, 0.386475848235596 );
	_inputQuery.storeDebuggingSolution( 62, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 63, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 64, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 65, 0.539427529702487 );
	_inputQuery.storeDebuggingSolution( 66, 0.604806084637551 );
	_inputQuery.storeDebuggingSolution( 67, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 68, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 69, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 70, 0.586885487448293 );
	_inputQuery.storeDebuggingSolution( 71, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 72, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 73, 0.929427224390576 );
	_inputQuery.storeDebuggingSolution( 74, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 75, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 76, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 77, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 78, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 79, 0.152049505785323 );
	_inputQuery.storeDebuggingSolution( 80, 0.180355112274895 );
	_inputQuery.storeDebuggingSolution( 81, 0.749208490962057 );
	_inputQuery.storeDebuggingSolution( 82, 0.018891219869575 );
	_inputQuery.storeDebuggingSolution( 83, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 84, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 85, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 86, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 87, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 88, 0.504430669661189 );
	_inputQuery.storeDebuggingSolution( 89, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 90, 0.080664708103537 );
	_inputQuery.storeDebuggingSolution( 91, 0.024257680041788 );
	_inputQuery.storeDebuggingSolution( 92, 0.133771175859501 );
	_inputQuery.storeDebuggingSolution( 93, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 94, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 95, 0.149514899259178 );
	_inputQuery.storeDebuggingSolution( 96, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 97, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 98, 0.228269589700240 );
	_inputQuery.storeDebuggingSolution( 99, 0.021731105194653 );
	_inputQuery.storeDebuggingSolution( 100, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 101, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 102, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 103, 0.273974269681440 );
	_inputQuery.storeDebuggingSolution( 104, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 105, -1.814097194621235 );
	_inputQuery.storeDebuggingSolution( 106, -3.485711290064927 );
	_inputQuery.storeDebuggingSolution( 107, 1.980928916282680 );
	_inputQuery.storeDebuggingSolution( 108, 0.100210540672761 );
	_inputQuery.storeDebuggingSolution( 109, 0.934858098351943 );
	_inputQuery.storeDebuggingSolution( 110, -0.631702118668596 );
	_inputQuery.storeDebuggingSolution( 111, -1.594437597530942 );
	_inputQuery.storeDebuggingSolution( 112, -0.358151799995413 );
	_inputQuery.storeDebuggingSolution( 113, -0.134942948144626 );
	_inputQuery.storeDebuggingSolution( 114, 0.409012097176247 );
	_inputQuery.storeDebuggingSolution( 115, -1.167872114452797 );
	_inputQuery.storeDebuggingSolution( 116, -0.874918493922043 );
	_inputQuery.storeDebuggingSolution( 117, 0.496477408692150 );
	_inputQuery.storeDebuggingSolution( 118, -2.976104229965346 );
	_inputQuery.storeDebuggingSolution( 119, 0.468623526799407 );
	_inputQuery.storeDebuggingSolution( 120, -0.169032907504973 );
	_inputQuery.storeDebuggingSolution( 121, -0.840452041445771 );
	_inputQuery.storeDebuggingSolution( 122, -1.196817780980797 );
	_inputQuery.storeDebuggingSolution( 123, -1.276840988107807 );
	_inputQuery.storeDebuggingSolution( 124, 0.467780060352692 );
	_inputQuery.storeDebuggingSolution( 125, 0.180057910047149 );
	_inputQuery.storeDebuggingSolution( 126, -1.787123247102062 );
	_inputQuery.storeDebuggingSolution( 127, 0.104962444267516 );
	_inputQuery.storeDebuggingSolution( 128, -0.059924219221870 );
	_inputQuery.storeDebuggingSolution( 129, -0.110847708717097 );
	_inputQuery.storeDebuggingSolution( 130, 0.484279334525501 );
	_inputQuery.storeDebuggingSolution( 131, -2.137190597996714 );
	_inputQuery.storeDebuggingSolution( 132, -1.278712546186011 );
	_inputQuery.storeDebuggingSolution( 133, -0.765786961972342 );
	_inputQuery.storeDebuggingSolution( 134, -0.330232456107896 );
	_inputQuery.storeDebuggingSolution( 135, -2.249821528568396 );
	_inputQuery.storeDebuggingSolution( 136, -1.653812312294233 );
	_inputQuery.storeDebuggingSolution( 137, -2.140165079363594 );
	_inputQuery.storeDebuggingSolution( 138, 0.438075258911679 );
	_inputQuery.storeDebuggingSolution( 139, 0.433800321443967 );
	_inputQuery.storeDebuggingSolution( 140, -2.732629250117331 );
	_inputQuery.storeDebuggingSolution( 141, 0.403523815971081 );
	_inputQuery.storeDebuggingSolution( 142, 1.962814704571039 );
	_inputQuery.storeDebuggingSolution( 143, -0.867589356005893 );
	_inputQuery.storeDebuggingSolution( 144, -2.562607740793648 );
	_inputQuery.storeDebuggingSolution( 145, -0.139130832789841 );
	_inputQuery.storeDebuggingSolution( 146, -0.197173911630577 );
	_inputQuery.storeDebuggingSolution( 147, -0.422330423597104 );
	_inputQuery.storeDebuggingSolution( 148, -1.587244258035277 );
	_inputQuery.storeDebuggingSolution( 149, -1.082530117909587 );
	_inputQuery.storeDebuggingSolution( 150, -2.406739474574185 );
	_inputQuery.storeDebuggingSolution( 151, -1.229938243197572 );
	_inputQuery.storeDebuggingSolution( 152, -3.982028610545695 );
	_inputQuery.storeDebuggingSolution( 153, 0.440399841544039 );
	_inputQuery.storeDebuggingSolution( 154, -1.167324914985042 );
	_inputQuery.storeDebuggingSolution( 155, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 156, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 157, 1.980928916282680 );
	_inputQuery.storeDebuggingSolution( 158, 0.100210540672761 );
	_inputQuery.storeDebuggingSolution( 159, 0.934858098351943 );
	_inputQuery.storeDebuggingSolution( 160, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 161, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 162, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 163, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 164, 0.409012097176247 );
	_inputQuery.storeDebuggingSolution( 165, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 166, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 167, 0.496477408692150 );
	_inputQuery.storeDebuggingSolution( 168, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 169, 0.468623526799407 );
	_inputQuery.storeDebuggingSolution( 170, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 171, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 172, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 173, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 174, 0.467780060352692 );
	_inputQuery.storeDebuggingSolution( 175, 0.180057910047149 );
	_inputQuery.storeDebuggingSolution( 176, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 177, 0.104962444267516 );
	_inputQuery.storeDebuggingSolution( 178, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 179, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 180, 0.484279334525501 );
	_inputQuery.storeDebuggingSolution( 181, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 182, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 183, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 184, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 185, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 186, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 187, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 188, 0.438075258911679 );
	_inputQuery.storeDebuggingSolution( 189, 0.433800321443967 );
	_inputQuery.storeDebuggingSolution( 190, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 191, 0.403523815971081 );
	_inputQuery.storeDebuggingSolution( 192, 1.962814704571039 );
	_inputQuery.storeDebuggingSolution( 193, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 194, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 195, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 196, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 197, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 198, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 199, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 200, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 201, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 202, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 203, 0.440399841544039 );
	_inputQuery.storeDebuggingSolution( 204, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 205, -0.819841839449259 );
	_inputQuery.storeDebuggingSolution( 206, -3.339588546604212 );
	_inputQuery.storeDebuggingSolution( 207, 0.954338732327055 );
	_inputQuery.storeDebuggingSolution( 208, -1.916987568306916 );
	_inputQuery.storeDebuggingSolution( 209, -0.722592611734338 );
	_inputQuery.storeDebuggingSolution( 210, 1.195181031791307 );
	_inputQuery.storeDebuggingSolution( 211, -0.855817204945971 );
	_inputQuery.storeDebuggingSolution( 212, 1.753778399076462 );
	_inputQuery.storeDebuggingSolution( 213, -0.157918520044487 );
	_inputQuery.storeDebuggingSolution( 214, -0.718464616586208 );
	_inputQuery.storeDebuggingSolution( 215, -2.065537780874092 );
	_inputQuery.storeDebuggingSolution( 216, -0.827796159892811 );
	_inputQuery.storeDebuggingSolution( 217, -1.225802621605897 );
	_inputQuery.storeDebuggingSolution( 218, -0.248544585219471 );
	_inputQuery.storeDebuggingSolution( 219, -0.493570370261531 );
	_inputQuery.storeDebuggingSolution( 220, -3.272381591809008 );
	_inputQuery.storeDebuggingSolution( 221, 0.268077428700418 );
	_inputQuery.storeDebuggingSolution( 222, -2.980293386803514 );
	_inputQuery.storeDebuggingSolution( 223, -0.282290393412418 );
	_inputQuery.storeDebuggingSolution( 224, -2.604777654191355 );
	_inputQuery.storeDebuggingSolution( 225, -0.258793141335357 );
	_inputQuery.storeDebuggingSolution( 226, -4.326068914847315 );
	_inputQuery.storeDebuggingSolution( 227, -0.268197296678169 );
	_inputQuery.storeDebuggingSolution( 228, -0.390878394876889 );
	_inputQuery.storeDebuggingSolution( 229, -1.008042684039985 );
	_inputQuery.storeDebuggingSolution( 230, -1.803256547029548 );
	_inputQuery.storeDebuggingSolution( 231, 1.166823121040047 );
	_inputQuery.storeDebuggingSolution( 232, 0.287174146272192 );
	_inputQuery.storeDebuggingSolution( 233, 0.753453392706852 );
	_inputQuery.storeDebuggingSolution( 234, 1.050290541223954 );
	_inputQuery.storeDebuggingSolution( 235, -2.105342233072897 );
	_inputQuery.storeDebuggingSolution( 236, 4.408440533653835 );
	_inputQuery.storeDebuggingSolution( 237, -0.235215420549355 );
	_inputQuery.storeDebuggingSolution( 238, -0.884934024834170 );
	_inputQuery.storeDebuggingSolution( 239, -4.392155789176237 );
	_inputQuery.storeDebuggingSolution( 240, -0.078040814602227 );
	_inputQuery.storeDebuggingSolution( 241, -0.212180312879921 );
	_inputQuery.storeDebuggingSolution( 242, -0.921775680124887 );
	_inputQuery.storeDebuggingSolution( 243, -2.215243813270182 );
	_inputQuery.storeDebuggingSolution( 244, -1.832681935440806 );
	_inputQuery.storeDebuggingSolution( 245, -0.009808218529067 );
	_inputQuery.storeDebuggingSolution( 246, -0.190348038056868 );
	_inputQuery.storeDebuggingSolution( 247, -3.518953290966444 );
	_inputQuery.storeDebuggingSolution( 248, 0.967470043259506 );
	_inputQuery.storeDebuggingSolution( 249, -0.933365168677610 );
	_inputQuery.storeDebuggingSolution( 250, -0.072745080859074 );
	_inputQuery.storeDebuggingSolution( 251, -0.248205211848387 );
	_inputQuery.storeDebuggingSolution( 252, -1.308738869063178 );
	_inputQuery.storeDebuggingSolution( 253, 0.344026972276948 );
	_inputQuery.storeDebuggingSolution( 254, -0.074306313074304 );
	_inputQuery.storeDebuggingSolution( 255, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 256, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 257, 0.954338732327055 );
	_inputQuery.storeDebuggingSolution( 258, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 259, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 260, 1.195181031791307 );
	_inputQuery.storeDebuggingSolution( 261, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 262, 1.753778399076462 );
	_inputQuery.storeDebuggingSolution( 263, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 264, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 265, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 266, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 267, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 268, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 269, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 270, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 271, 0.268077428700418 );
	_inputQuery.storeDebuggingSolution( 272, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 273, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 274, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 275, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 276, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 277, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 278, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 279, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 280, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 281, 1.166823121040047 );
	_inputQuery.storeDebuggingSolution( 282, 0.287174146272192 );
	_inputQuery.storeDebuggingSolution( 283, 0.753453392706852 );
	_inputQuery.storeDebuggingSolution( 284, 1.050290541223954 );
	_inputQuery.storeDebuggingSolution( 285, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 286, 4.408440533653835 );
	_inputQuery.storeDebuggingSolution( 287, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 288, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 289, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 290, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 291, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 292, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 293, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 294, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 295, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 296, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 297, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 298, 0.967470043259506 );
	_inputQuery.storeDebuggingSolution( 299, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 300, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 301, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 302, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 303, 0.344026972276948 );
	_inputQuery.storeDebuggingSolution( 304, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 305, -0.598714282899663 );
	_inputQuery.storeDebuggingSolution( 306, 0.329905458207074 );
	_inputQuery.storeDebuggingSolution( 307, -0.609802157793882 );
	_inputQuery.storeDebuggingSolution( 308, -2.173770652864562 );
	_inputQuery.storeDebuggingSolution( 309, 1.461839404347806 );
	_inputQuery.storeDebuggingSolution( 310, -3.115937586938983 );
	_inputQuery.storeDebuggingSolution( 311, -1.549532674015744 );
	_inputQuery.storeDebuggingSolution( 312, -0.772572255979929 );
	_inputQuery.storeDebuggingSolution( 313, -0.109493765906076 );
	_inputQuery.storeDebuggingSolution( 314, -2.405993794903616 );
	_inputQuery.storeDebuggingSolution( 315, -1.484527101010831 );
	_inputQuery.storeDebuggingSolution( 316, -1.588240224640874 );
	_inputQuery.storeDebuggingSolution( 317, 0.850258098967325 );
	_inputQuery.storeDebuggingSolution( 318, -1.969586033362603 );
	_inputQuery.storeDebuggingSolution( 319, -0.632421143978703 );
	_inputQuery.storeDebuggingSolution( 320, -1.290415613422695 );
	_inputQuery.storeDebuggingSolution( 321, -0.476531163578377 );
	_inputQuery.storeDebuggingSolution( 322, 1.019189020544671 );
	_inputQuery.storeDebuggingSolution( 323, 0.721440283772089 );
	_inputQuery.storeDebuggingSolution( 324, -0.385387519084386 );
	_inputQuery.storeDebuggingSolution( 325, 1.179732460495737 );
	_inputQuery.storeDebuggingSolution( 326, 0.261889654937604 );
	_inputQuery.storeDebuggingSolution( 327, 1.044266220114027 );
	_inputQuery.storeDebuggingSolution( 328, -0.517781000171870 );
	_inputQuery.storeDebuggingSolution( 329, -0.440547635658763 );
	_inputQuery.storeDebuggingSolution( 330, -0.201031511401373 );
	_inputQuery.storeDebuggingSolution( 331, 1.729745891861604 );
	_inputQuery.storeDebuggingSolution( 332, 2.650982659416633 );
	_inputQuery.storeDebuggingSolution( 333, 0.958100712220294 );
	_inputQuery.storeDebuggingSolution( 334, -1.788634355090187 );
	_inputQuery.storeDebuggingSolution( 335, -4.652222905819809 );
	_inputQuery.storeDebuggingSolution( 336, 3.851734871351951 );
	_inputQuery.storeDebuggingSolution( 337, 0.551975665054400 );
	_inputQuery.storeDebuggingSolution( 338, -2.644825616647230 );
	_inputQuery.storeDebuggingSolution( 339, -0.386004246223271 );
	_inputQuery.storeDebuggingSolution( 340, -0.330930410145224 );
	_inputQuery.storeDebuggingSolution( 341, -1.326175905729980 );
	_inputQuery.storeDebuggingSolution( 342, -0.393283301240311 );
	_inputQuery.storeDebuggingSolution( 343, 0.781076241750634 );
	_inputQuery.storeDebuggingSolution( 344, -3.345190365808053 );
	_inputQuery.storeDebuggingSolution( 345, 3.082795931442887 );
	_inputQuery.storeDebuggingSolution( 346, -3.656191453552038 );
	_inputQuery.storeDebuggingSolution( 347, 0.628313326294148 );
	_inputQuery.storeDebuggingSolution( 348, -4.151937836341113 );
	_inputQuery.storeDebuggingSolution( 349, -2.323119126940298 );
	_inputQuery.storeDebuggingSolution( 350, -1.823720536531878 );
	_inputQuery.storeDebuggingSolution( 351, -2.579303460821195 );
	_inputQuery.storeDebuggingSolution( 352, -0.766474004781044 );
	_inputQuery.storeDebuggingSolution( 353, -1.569298705890125 );
	_inputQuery.storeDebuggingSolution( 354, 1.142530789608453 );
	_inputQuery.storeDebuggingSolution( 355, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 356, 0.329905458207074 );
	_inputQuery.storeDebuggingSolution( 357, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 358, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 359, 1.461839404347806 );
	_inputQuery.storeDebuggingSolution( 360, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 361, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 362, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 363, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 364, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 365, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 366, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 367, 0.850258098967325 );
	_inputQuery.storeDebuggingSolution( 368, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 369, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 370, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 371, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 372, 1.019189020544671 );
	_inputQuery.storeDebuggingSolution( 373, 0.721440283772089 );
	_inputQuery.storeDebuggingSolution( 374, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 375, 1.179732460495737 );
	_inputQuery.storeDebuggingSolution( 376, 0.261889654937604 );
	_inputQuery.storeDebuggingSolution( 377, 1.044266220114027 );
	_inputQuery.storeDebuggingSolution( 378, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 379, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 380, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 381, 1.729745891861604 );
	_inputQuery.storeDebuggingSolution( 382, 2.650982659416633 );
	_inputQuery.storeDebuggingSolution( 383, 0.958100712220294 );
	_inputQuery.storeDebuggingSolution( 384, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 385, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 386, 3.851734871351951 );
	_inputQuery.storeDebuggingSolution( 387, 0.551975665054400 );
	_inputQuery.storeDebuggingSolution( 388, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 389, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 390, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 391, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 392, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 393, 0.781076241750634 );
	_inputQuery.storeDebuggingSolution( 394, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 395, 3.082795931442887 );
	_inputQuery.storeDebuggingSolution( 396, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 397, 0.628313326294148 );
	_inputQuery.storeDebuggingSolution( 398, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 399, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 400, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 401, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 402, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 403, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 404, 1.142530789608453 );
	_inputQuery.storeDebuggingSolution( 405, 4.428938018243580 );
	_inputQuery.storeDebuggingSolution( 406, -2.309234031537470 );
	_inputQuery.storeDebuggingSolution( 407, -0.330276625371428 );
	_inputQuery.storeDebuggingSolution( 408, -26.287383021132396 );
	_inputQuery.storeDebuggingSolution( 409, -5.250671885990315 );
	_inputQuery.storeDebuggingSolution( 410, -1.887862318310334 );
	_inputQuery.storeDebuggingSolution( 411, -1.351130583252519 );
	_inputQuery.storeDebuggingSolution( 412, -1.062951063504593 );
	_inputQuery.storeDebuggingSolution( 413, -0.747921081292743 );
	_inputQuery.storeDebuggingSolution( 414, -2.094169471098706 );
	_inputQuery.storeDebuggingSolution( 415, 0.596254169071571 );
	_inputQuery.storeDebuggingSolution( 416, -8.716239150003403 );
	_inputQuery.storeDebuggingSolution( 417, -8.011469640692345 );
	_inputQuery.storeDebuggingSolution( 418, -2.238514777344007 );
	_inputQuery.storeDebuggingSolution( 419, -2.554301521761621 );
	_inputQuery.storeDebuggingSolution( 420, -3.518990045433296 );
	_inputQuery.storeDebuggingSolution( 421, -0.436436201604759 );
	_inputQuery.storeDebuggingSolution( 422, -1.967010749018098 );
	_inputQuery.storeDebuggingSolution( 423, -0.394422112935743 );
	_inputQuery.storeDebuggingSolution( 424, -0.697398390650676 );
	_inputQuery.storeDebuggingSolution( 425, 0.819517533143082 );
	_inputQuery.storeDebuggingSolution( 426, -1.537118933952041 );
	_inputQuery.storeDebuggingSolution( 427, -0.682551464634356 );
	_inputQuery.storeDebuggingSolution( 428, -3.949996363230209 );
	_inputQuery.storeDebuggingSolution( 429, -1.763136591162025 );
	_inputQuery.storeDebuggingSolution( 430, 1.330835515182759 );
	_inputQuery.storeDebuggingSolution( 431, -8.801603920601064 );
	_inputQuery.storeDebuggingSolution( 432, 0.318706733102563 );
	_inputQuery.storeDebuggingSolution( 433, -5.877346615436542 );
	_inputQuery.storeDebuggingSolution( 434, -1.741775036557589 );
	_inputQuery.storeDebuggingSolution( 435, 3.265424889045409 );
	_inputQuery.storeDebuggingSolution( 436, -3.546161196908778 );
	_inputQuery.storeDebuggingSolution( 437, 0.200156929046506 );
	_inputQuery.storeDebuggingSolution( 438, -0.787195362295452 );
	_inputQuery.storeDebuggingSolution( 439, 3.617581214862575 );
	_inputQuery.storeDebuggingSolution( 440, -0.762387858013354 );
	_inputQuery.storeDebuggingSolution( 441, -2.280002557332472 );
	_inputQuery.storeDebuggingSolution( 442, 0.814066106815785 );
	_inputQuery.storeDebuggingSolution( 443, -1.101404876628545 );
	_inputQuery.storeDebuggingSolution( 444, 2.040333005128947 );
	_inputQuery.storeDebuggingSolution( 445, -0.177254463101125 );
	_inputQuery.storeDebuggingSolution( 446, -1.006106366497315 );
	_inputQuery.storeDebuggingSolution( 447, -0.720388047167368 );
	_inputQuery.storeDebuggingSolution( 448, -1.293390389826994 );
	_inputQuery.storeDebuggingSolution( 449, 3.372234452959631 );
	_inputQuery.storeDebuggingSolution( 450, 0.068160757251724 );
	_inputQuery.storeDebuggingSolution( 451, 0.090125721820875 );
	_inputQuery.storeDebuggingSolution( 452, -1.110411576638461 );
	_inputQuery.storeDebuggingSolution( 453, -9.138622945439096 );
	_inputQuery.storeDebuggingSolution( 454, 2.194173416983260 );
	_inputQuery.storeDebuggingSolution( 455, 4.428938018243580 );
	_inputQuery.storeDebuggingSolution( 456, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 457, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 458, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 459, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 460, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 461, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 462, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 463, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 464, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 465, 0.596254169071571 );
	_inputQuery.storeDebuggingSolution( 466, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 467, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 468, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 469, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 470, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 471, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 472, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 473, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 474, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 475, 0.819517533143082 );
	_inputQuery.storeDebuggingSolution( 476, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 477, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 478, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 479, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 480, 1.330835515182759 );
	_inputQuery.storeDebuggingSolution( 481, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 482, 0.318706733102563 );
	_inputQuery.storeDebuggingSolution( 483, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 484, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 485, 3.265424889045409 );
	_inputQuery.storeDebuggingSolution( 486, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 487, 0.200156929046506 );
	_inputQuery.storeDebuggingSolution( 488, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 489, 3.617581214862575 );
	_inputQuery.storeDebuggingSolution( 490, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 491, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 492, 0.814066106815785 );
	_inputQuery.storeDebuggingSolution( 493, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 494, 2.040333005128947 );
	_inputQuery.storeDebuggingSolution( 495, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 496, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 497, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 498, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 499, 3.372234452959631 );
	_inputQuery.storeDebuggingSolution( 500, 0.068160757251724 );
	_inputQuery.storeDebuggingSolution( 501, 0.090125721820875 );
	_inputQuery.storeDebuggingSolution( 502, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 503, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 504, 2.194173416983260 );
	_inputQuery.storeDebuggingSolution( 505, -0.397166681110892 );
	_inputQuery.storeDebuggingSolution( 506, -1.558103984527249 );
	_inputQuery.storeDebuggingSolution( 507, -10.266601180057354 );
	_inputQuery.storeDebuggingSolution( 508, -0.577633777322390 );
	_inputQuery.storeDebuggingSolution( 509, -13.909615245196964 );
	_inputQuery.storeDebuggingSolution( 510, -1.341170974284360 );
	_inputQuery.storeDebuggingSolution( 511, -4.654392137697649 );
	_inputQuery.storeDebuggingSolution( 512, -1.332482370765273 );
	_inputQuery.storeDebuggingSolution( 513, -0.249517829542704 );
	_inputQuery.storeDebuggingSolution( 514, -5.335972113929813 );
	_inputQuery.storeDebuggingSolution( 515, -0.864832120672533 );
	_inputQuery.storeDebuggingSolution( 516, -0.363312298334206 );
	_inputQuery.storeDebuggingSolution( 517, -9.851442751812897 );
	_inputQuery.storeDebuggingSolution( 518, -2.877148933205924 );
	_inputQuery.storeDebuggingSolution( 519, -0.575068382514299 );
	_inputQuery.storeDebuggingSolution( 520, -2.610568202241871 );
	_inputQuery.storeDebuggingSolution( 521, -0.363846900273581 );
	_inputQuery.storeDebuggingSolution( 522, -0.658933384047957 );
	_inputQuery.storeDebuggingSolution( 523, -0.428313392723389 );
	_inputQuery.storeDebuggingSolution( 524, -4.850634626000871 );
	_inputQuery.storeDebuggingSolution( 525, -0.629341586618490 );
	_inputQuery.storeDebuggingSolution( 526, -1.998909912326098 );
	_inputQuery.storeDebuggingSolution( 527, -3.797835372439853 );
	_inputQuery.storeDebuggingSolution( 528, -1.906757688150976 );
	_inputQuery.storeDebuggingSolution( 529, -6.931095328472358 );
	_inputQuery.storeDebuggingSolution( 530, -12.948970819833386 );
	_inputQuery.storeDebuggingSolution( 531, -1.201517561161421 );
	_inputQuery.storeDebuggingSolution( 532, -3.579275250743406 );
	_inputQuery.storeDebuggingSolution( 533, -3.760713892806497 );
	_inputQuery.storeDebuggingSolution( 534, -0.635416351363822 );
	_inputQuery.storeDebuggingSolution( 535, -8.510843271761352 );
	_inputQuery.storeDebuggingSolution( 536, -2.000760589010298 );
	_inputQuery.storeDebuggingSolution( 537, -0.231544779250410 );
	_inputQuery.storeDebuggingSolution( 538, -5.862908216353206 );
	_inputQuery.storeDebuggingSolution( 539, -0.589497646544356 );
	_inputQuery.storeDebuggingSolution( 540, -1.086215435193294 );
	_inputQuery.storeDebuggingSolution( 541, -3.256037281594390 );
	_inputQuery.storeDebuggingSolution( 542, 0.097756454008896 );
	_inputQuery.storeDebuggingSolution( 543, 0.059418681036464 );
	_inputQuery.storeDebuggingSolution( 544, -0.755960098460409 );
	_inputQuery.storeDebuggingSolution( 545, 1.628061225890116 );
	_inputQuery.storeDebuggingSolution( 546, -0.064842564682349 );
	_inputQuery.storeDebuggingSolution( 547, -0.504832622358698 );
	_inputQuery.storeDebuggingSolution( 548, -4.103682351344862 );
	_inputQuery.storeDebuggingSolution( 549, -1.949696708010904 );
	_inputQuery.storeDebuggingSolution( 550, -1.073845641675230 );
	_inputQuery.storeDebuggingSolution( 551, -1.628119558605466 );
	_inputQuery.storeDebuggingSolution( 552, -0.889858432718650 );
	_inputQuery.storeDebuggingSolution( 553, -0.890037716699046 );
	_inputQuery.storeDebuggingSolution( 554, -0.579376840195014 );
	_inputQuery.storeDebuggingSolution( 555, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 556, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 557, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 558, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 559, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 560, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 561, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 562, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 563, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 564, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 565, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 566, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 567, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 568, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 569, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 570, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 571, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 572, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 573, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 574, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 575, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 576, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 577, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 578, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 579, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 580, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 581, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 582, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 583, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 584, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 585, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 586, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 587, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 588, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 589, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 590, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 591, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 592, 0.097756454008896 );
	_inputQuery.storeDebuggingSolution( 593, 0.059418681036464 );
	_inputQuery.storeDebuggingSolution( 594, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 595, 1.628061225890116 );
	_inputQuery.storeDebuggingSolution( 596, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 597, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 598, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 599, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 600, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 601, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 602, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 603, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 604, 0.000000000000000 );
	_inputQuery.storeDebuggingSolution( 605, 0.027857442044825 );
	_inputQuery.storeDebuggingSolution( 606, -0.020643886364881 );
	_inputQuery.storeDebuggingSolution( 607, 0.021868257688458 );
	_inputQuery.storeDebuggingSolution( 608, -0.012566325246726 );
	_inputQuery.storeDebuggingSolution( 609, 0.022523649758401 );
    */
    //Output,0.0278574 -0.0206439 0.0218683 -0.0125663 0.0225236 )

}

void Marabou::solveQuery()
{
    if ( _engine.processInputQuery( _inputQuery ) )
        _engine.solve();

    if ( _engine.getExitCode() == Engine::SAT )
        _engine.extractSolution( _inputQuery );
}

void Marabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    Engine::ExitCode result = _engine.getExitCode();
    String resultString;

    if ( result == Engine::UNSAT )
    {
        resultString = "UNSAT";
        printf( "UNSAT\n" );
    }
    else if ( result == Engine::SAT )
    {
        resultString = "SAT";
        printf( "SAT\n\n" );

        printf( "Input assignment:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumInputVariables(); ++i )
            printf( "\tx%u = %8.4lf\n", i, _inputQuery.getSolutionValue( _inputQuery.inputVariableByIndex( i ) ) );

        printf( "\n" );
        printf( "Output:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumOutputVariables(); ++i )
            printf( "\ty%u = %8.4lf\n", i, _inputQuery.getSolutionValue( _inputQuery.outputVariableByIndex( i ) ) );
        printf( "\n" );


        printf( "\n" );
        printf( "Full assignment:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumberOfVariables(); ++i )
            printf( "\tx%u = %.15lf\n", i, _inputQuery.getSolutionValue( i ) );

        printf( "\n" );
        printf( "\n" );

    }
    else if ( result == Engine::TIMEOUT )
    {
        resultString = "TIMEOUT";
        printf( "Timeout\n" );
    }
    else if ( result == Engine::ERROR )
    {
        resultString = "ERROR";
        printf( "Error\n" );
    }
    else
    {
        resultString = "UNKNOWN";
        printf( "UNKNOWN EXIT CODE! (this should not happen)" );
    }

    // Create a summary file, if requested
    String summaryFilePath = Options::get()->getString( Options::SUMMARY_FILE );
    if ( summaryFilePath != "" )
    {
        File summaryFile( summaryFilePath );
        summaryFile.open( File::MODE_WRITE_TRUNCATE );

        // Field #1: result
        summaryFile.write( resultString );

        // Field #2: total elapsed time
        summaryFile.write( Stringf( " %u ", microSecondsElapsed / 1000000 ) ); // In seconds

        // Field #3: number of visited tree states
        summaryFile.write( Stringf( "%u ",
                                    _engine.getStatistics()->getNumVisitedTreeStates() ) );

        // Field #4: average pivot time in micro seconds
        summaryFile.write( Stringf( "%u",
                                    _engine.getStatistics()->getAveragePivotTimeInMicro() ) );

        summaryFile.write( "\n" );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
