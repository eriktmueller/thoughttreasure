# thoughttreasure
ThoughtTreasure commonsense knowledge base and architecture for natural language processing

http://www.amazon.com/gp/product/1478171650/

Functionality

    Comprehensive natural language processing/commonsense platform
    Question answering
    Chatterbot
    Story understanding
    Classified ad parsing
    Simulation of a 2-dimensional virtual world
    Identification of entities in text
    Information extraction from ASCII tables
    Concordance generation
    Shell (command interpreter)
    Web API
    Java-based client API
    Python-based client API
    Knowledge base flat files

Applications

    Commonsense reasoning
    Question answering
    Information extraction
    Interactive fiction

Source Code

    Language: ANSI C
    Lines of code including comments: 75,000

Knowledge base and lexicon

    Upper ontology
    Domain lower ontologies (clothing, food, music, and so on)
    Concepts, words, and phrases defined using concise language
    English words and phrases: 35,000
    French words and phrases: 20,000
    Closed-class lexical items not in WordNet (adverbs and adverbials, prepositions, determiners, pronouns, conjunctions, interjections, temporal relations, and so on)
    Concepts: 25,000
    Assertions: 50,000 including hierarchical links
    Scripts: 100 with events and roles

Linguistic features

    English and French lexicon/ontology
    Verb/nominalization argument structure
    Name dictionary with grouping of related names
    Text agents: part-of-speech tagging, identification of names, places, products, dates, phone numbers, email headers
    English and French syntactic parser: base component, filters, transformations
    English and French semantic parser: intension/extension, relative clauses, appositives, genitives, tense/aspect
    Anaphoric parser: deixis, determiners, pronouns, c-command
    Understanding agents: converting surfacy parse into detailed understanding, steering planning agents, contexts, emotions, goals, question answering, asking clarifying questions, appointments, sleep, grids by analogy
    English and French generator
    Learning of new words using derivational rules
    Learning of new inflections by algorithmic/analogical morphology

Simulation features

    Space represented by 2-dimensional grids connected by wormholes
    Prototypical grids: house, apartment, restaurant, theater, street, subway
    Planning agents for simulating human behavior: graspers, containers, ptrans++, atrans++, mtrans++, interpersonal relations
    Planning agents for simulating device behavior: telephone, television

Procedures

    Database: Assertion/retrieval of assertions
    Contexts
    Anagram finder
    Palindrome finder
    Inverted dictionary generator
    English/French faux ami finder
    Intension resolver: find objects matching a description
    Subgoaling
    2-dimensional (occupancy array) path planner
    Grid operations (distance, subspace, etc.)
    Intergrid path planner
    Trip planner
    Clothing color matching
    Operations for parts and wholes of objects
    Operations for nested space (room, floor, building, city, planet)
    Operations for large space (planetary distance, polity containment)
    Theorem prover
    Temporal projection
    Abduction
    Relation learner (corporate successions)
    Assertion learner
    Language translation
    Dictionary generator
    Free association generator
    Corpus analysis tools
    Multi-language, multi-stream discourse channel
    Chatterbot main loop
    Typing simulator with errors
    HTML utilities
    Report generation
