graph [ id 1
creator "marco"
directed 1
node [ id 0  ]
node [ id 1  ]
node [ id 2  ]
node [ id 3  ]
node [ id 4 ]
node [ id 5 ]
node [ id 6 ]

node [ id 7 ]
node [ id 8 ]
node [ id 9 ]
node [ id 10 ]

edge [ source 1 target 0 label "--+1" ]
edge [ source 1 target 2 label "++-1q" ]

edge [ source 10 target 2 label "+-+pw(2,0)p!1pw(1,0)" ]
edge [ source 10 target 2 label "+++pw(2,0)p!1q" ]

edge [ source 9 target 2 label "+-+pw(2,0)q" ]
edge [ source 9 target 3 label "+++1" ]

edge [ source 8 target 3 label "+++!1q" ]
edge [ source 8 target 2 label "+-+qq" ]

edge [ source 7 target 2 label "+-+pw(3,1)" ]
edge [ source 7 target 3 label "++-!1p" ]

edge [ source 4 target 2 label "+--ppw(6,2)!1q" ]
edge [ source 4 target 2 label "++-pq" ]

edge [ source 5 target 2 label "+--ppw(6,2)!1p" ]
edge [ source 5 target 2 label "++-ppw(5,1)!1q" ]

edge [ source 6 target 2 label "+--ppw(5,1)!1ppw(4,0)" ]
edge [ source 6 target 2 label "++-ppw(5,1)!1pq" ]

]

