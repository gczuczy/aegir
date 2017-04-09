import { Component, OnInit } from '@angular/core';

import { Program } from '../program';

@Component({
  templateUrl: './add-program.component.html',
  styleUrls: ['./add-program.component.css']
})
export class AddProgramComponent implements OnInit {
    fornName = 'Add a component';

    program: Program = new Program(null, '');

    constructor() {
	console.log('ctor');
    }

    ngOnInit() {
	program = new Program(null, '');
	console.log('e!');
    }

    onSubmit() {
	console.log('e! ', JSON.stringify(this.program));
    };

}
